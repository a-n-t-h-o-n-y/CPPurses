#ifndef TERMOX_WIDGET_WIDGETS_BANNER_HPP
#define TERMOX_WIDGET_WIDGETS_BANNER_HPP
#include <chrono>
#include <cstddef>
#include <numeric>
#include <optional>
#include <random>
#include <utility>
#include <vector>

#include <signals_light/signal.hpp>

#include <termox/painter/glyph_string.hpp>
#include <termox/painter/painter.hpp>
#include <termox/painter/palette/dawn_bringer16.hpp>
#include <termox/system/animation_engine.hpp>
#include <termox/system/system.hpp>
#include <termox/terminal/terminal.hpp>
#include <termox/widget/pipe.hpp>
#include <termox/widget/widget.hpp>

namespace ox {

struct Index_and_position {
    std::size_t index;
    int position;
};

inline auto operator++(Index_and_position& x) -> Index_and_position&
{
    ++x.index;
    ++x.position;
    return x;
}

class IP_range {
   public:
    using Iter_t = std::vector<Index_and_position>::const_iterator;

   public:
    IP_range(Iter_t begin, Iter_t end) : begin_{begin}, end_{end} {}

   public:
    [[nodiscard]] auto begin() const -> Iter_t { return begin_; }

    [[nodiscard]] auto end() const -> Iter_t { return end_; }

   private:
    Iter_t begin_;
    Iter_t end_;
};

/// Animated Text Line/Label
template <typename Animator>
class Banner : public Widget {
   public:
    template <typename... Args>
    Banner(Animation_engine::Interval_t interval, Args&&... args)
        : period_{interval}, animator_{std::forward<Args>(args)...}
    {
        *this | pipe::fixed_height(1);
        animator_.start.connect([this] { this->start(); });
        animator_.stop.connect([this] { this->stop(); });
    }

   public:
    void set_text(Glyph_string text)
    {
        this->stop();
        text_ = std::move(text);
        range_.reset();
        animator_.set_text_length(this->text().size());
        this->update();
    }

    [[nodiscard]] auto text() const -> Glyph_string const& { return text_; }

   protected:
    auto paint_event(Painter& p) -> bool override
    {
        if (range_) {
            for (auto const& x : *range_)
                p.put(text_[x.index], {x.position, 0});
        }
        else
            p.put(text_, {0, 0});
        return Widget::paint_event(p);
    }

    auto timer_event() -> bool override
    {
        range_ = animator_();
        this->update();
        return Widget::timer_event();
    }

    auto resize_event(Area new_size, Area old_size) -> bool override
    {
        animator_.set_max_length(new_size.width);
        return Widget::resize_event(new_size, old_size);
    }

   private:
    Animation_engine::Interval_t const period_;
    Animator animator_;
    Glyph_string text_;
    std::optional<IP_range> range_;

   private:
    void start()
    {
        if (text_.size() == 0)
            return;
        this->enable_animation(period_);
    }

    void stop()
    {
        this->disable_animation();
        range_.reset();
    }
};

/// Helper function to create an instance.
template <typename Animator, typename... Args>
[[nodiscard]] auto banner(Args&&... args) -> std::unique_ptr<Banner<Animator>>
{
    return std::make_unique<Banner<Animator>>(std::forward<Args>(args)...);
}

}  // namespace ox

namespace ox::animator {

class Animator_base {
   public:
    sl::Signal<void()> start;
    sl::Signal<void()> stop;

   public:
    void set_max_length(std::size_t x) { max_length_ = x; }

    void set_text_length(std::size_t x) { text_length_ = x; }

   protected:
    Animator_base()
    {
        start.connect([this] { started_ = true; });
        stop.connect([this] { started_ = false; });
    }

   protected:
    [[nodiscard]] auto data() const -> std::vector<Index_and_position> const&
    {
        return data_;
    }

    [[nodiscard]] auto data() -> std::vector<Index_and_position>&
    {
        return data_;
    }

    /// Incremented values, length of text.
    void initialize_data()
    {
        this->data().resize(this->text_length());
        std::iota(this->begin(), this->end(), Index_and_position{0, 0});
    }

    [[nodiscard]] auto max_length() const -> std::size_t { return max_length_; }

    [[nodiscard]] auto text_length() const -> std::size_t
    {
        return text_length_;
    }

    [[nodiscard]] auto is_started() const -> bool { return started_; }

    [[nodiscard]] auto begin() -> std::vector<Index_and_position>::iterator
    {
        return std::begin(data_);
    }

    [[nodiscard]] auto begin() const
        -> std::vector<Index_and_position>::const_iterator
    {
        return std::cbegin(data_);
    }

    [[nodiscard]] auto end() -> std::vector<Index_and_position>::iterator
    {
        return std::end(data_);
    }

    [[nodiscard]] auto end() const
        -> std::vector<Index_and_position>::const_iterator
    {
        return std::cend(data_);
    }

   private:
    std::vector<Index_and_position> data_;
    std::size_t max_length_;
    std::size_t text_length_;
    bool started_ = false;
};

/* ------------------- Banner Animation Implementations ----------------------*/

/// Left to right reveal of text, hold, left to right clearning of text.
class Scan : public Animator_base {
   public:
    [[nodiscard]] auto operator()() -> IP_range
    {
        auto const t_length = this->text_length();
        auto const hold_max = t_length * 3;
        if (begin_ == 0 && end_ != t_length && hold_ == 0)
            ++end_;
        else if (begin_ == 0 && end_ == t_length && hold_ != hold_max)
            ++hold_;
        else if (begin_ != t_length && end_ == t_length && hold_ == hold_max)
            ++begin_;
        else
            stop();

        return {this->begin() + begin_, this->begin() + end_};
    }

    void set_text_length(std::size_t x)
    {
        Animator_base::set_text_length(x);
        if (this->text_length() == 0)
            return;
        this->Animator_base::initialize_data();
        begin_ = 0;
        end_   = 0;
        hold_  = 0;
        start();
    }

   private:
    std::size_t begin_;
    std::size_t end_;
    std::size_t hold_;
};

/// Left to right reveal of text, then hold.
class Persistent_scan : public Animator_base {
   public:
    [[nodiscard]] auto operator()() -> IP_range
    {
        if (end_ == this->text_length()) {
            stop();
            return {this->begin(), this->end()};
        }
        return {this->begin(), this->begin() + end_++};
    }

    void set_text_length(std::size_t x)
    {
        Animator_base::set_text_length(x);
        if (this->text_length() == 0)
            return;
        this->Animator_base::initialize_data();
        end_ = 0;
        start();
    }

   private:
    std::size_t end_;
};

class Random : public Animator_base {
   public:
    [[nodiscard]] auto operator()() -> IP_range
    {
        if (end_ == this->text_length()) {
            stop();
            return {this->begin(), this->end()};
        }
        return {this->begin(), this->begin() + end_++};
    }

    void set_text_length(std::size_t x)
    {
        Animator_base::set_text_length(x);
        if (this->text_length() == 0)
            return;
        this->Animator_base::initialize_data();
        shuffle(this->data());
        end_ = 0;
        start();
    }

   private:
    std::size_t end_;

   private:
    template <typename Container>
    static void shuffle(Container& container)
    {
        static auto gen_ = std::mt19937{std::random_device{}()};
        std::shuffle(std::begin(container), std::end(container), gen_);
    }
};

class Scroll_base : public Animator_base {
   public:
    [[nodiscard]] auto operator()() -> IP_range
    {
        if (hold_ < hold_length_)
            ++hold_;
        else if (begin_ == this->text_length()) {
            this->Animator_base::initialize_data();
            begin_ = 0;
            hold_  = 0;
        }
        else {
            ++begin_;
            std::for_each(this->begin() + begin_, this->end(),
                          [](auto& x) { --x.position; });
        }
        return {this->begin() + begin_, this->end()};
    }

    void set_text_length(std::size_t x)
    {
        Animator_base::set_text_length(x);
        this->reset_hold_length();
        this->Animator_base::initialize_data();
        begin_ = 0;
        hold_  = 0;
    }

    void set_max_length(std::size_t x)
    {
        Animator_base::set_max_length(x);
        this->reset_hold_length();
    }

   protected:
    std::size_t begin_ = 0;
    std::size_t hold_  = 0;

   private:
    void reset_hold_length()
    {
        // Heuristic
        hold_length_ =
            std::min({this->max_length(), this->text_length(), 20uL});
        hold_length_ = std::max(hold_length_, 20uL);
    }

   private:
    std::size_t hold_length_;
};

class Scroll : public Scroll_base {
   public:
    void set_text_length(std::size_t x)
    {
        Scroll_base::set_text_length(x);
        if (this->text_length() != 0)
            start();
    }
};

class Conditional_scroll : public Scroll_base {
   public:
    void set_text_length(std::size_t x)
    {
        Scroll_base::set_text_length(x);
        if (this->start_condition())
            start();
        else if (this->stop_condition())
            this->stop_and_reset();
    }

    void set_max_length(std::size_t x)
    {
        Scroll_base::set_max_length(x);
        if (this->start_condition())
            start();
        else if (this->stop_condition())
            this->stop_and_reset();
    }

   private:
    [[nodiscard]] auto start_condition() const -> bool
    {
        return !this->is_started() &&
               this->text_length() > this->max_length() &&
               this->text_length() != 0;
    }

    [[nodiscard]] auto stop_condition() const -> bool
    {
        return this->is_started() && this->text_length() <= this->max_length();
    }

    void stop_and_reset()
    {
        this->Scroll_base::initialize_data();
        hold_  = 0;
        begin_ = 0;
        stop();
    }
};

class Unscramble : public Animator_base {
   public:
    [[nodiscard]] auto operator()() -> IP_range
    {
        if (static_cast<std::size_t>(sorted_to_) == this->text_length()) {
            stop();
            return {this->begin(), this->end()};
        }
        else {
            // TODO concise rewrite
            for (std::size_t i = sorted_to_; i < this->data().size(); ++i) {
                auto& x = this->data()[i];
                if (x.position == sorted_to_) {
                    std::swap(x.position, this->data()[sorted_to_].position);
                    break;
                }
            }
            ++sorted_to_;
        }
        return {this->begin(), this->end()};
    }

    void set_text_length(std::size_t x)
    {
        Animator_base::set_text_length(x);
        if (this->text_length() == 0)
            return;
        this->initialize_data();
        sorted_to_ = 0;
        start();
    }

   private:
    int sorted_to_;

   private:
    void initialize_data()
    {
        auto indices   = std::vector<std::size_t>(this->text_length());
        auto positions = std::vector<int>(this->text_length());
        std::iota(std::begin(indices), std::end(indices), 0uL);
        std::iota(std::begin(positions), std::end(positions), 0);
        shuffle(positions);
        this->data() = zip<Index_and_position>(indices, positions);
    }

    template <typename Pair_t, typename Container1, typename Container2>
    [[nodiscard]] static auto zip(Container1 const& x, Container2 const& y)
        -> std::vector<Pair_t>
    {
        auto zipped = std::vector<Pair_t>{};
        for (auto i = 0uL; i < x.size(); ++i)
            zipped.push_back({x[i], y[i]});
        return zipped;
    }

    template <typename Container>
    static void shuffle(Container& container)
    {
        static auto gen_ = std::mt19937{std::random_device{}()};
        std::shuffle(std::begin(container), std::end(container), gen_);
    }
};

}  // namespace ox::animator
#endif  // TERMOX_WIDGET_WIDGETS_BANNER_HPP
