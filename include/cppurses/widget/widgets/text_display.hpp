#ifndef WIDGET_WIDGETS_TEXT_DISPLAY_HPP
#define WIDGET_WIDGETS_TEXT_DISPLAY_HPP
#include "painter/attribute.hpp"
#include "painter/brush.hpp"
#include "painter/glyph.hpp"
#include "painter/glyph_string.hpp"
#include "widget/coordinates.hpp"
#include "widget/widget.hpp"
#include <signals/signal.hpp>
#include <cstddef>
#include <vector>

namespace cppurses {

class Text_display : public Widget {
   public:
    explicit Text_display(Glyph_string content = "");

    // Text Modification
    void set_text(Glyph_string text);
    void insert(Glyph_string text, std::size_t index);
    void append(Glyph_string text);
    void erase(std::size_t index, std::size_t length = Glyph_string::npos);
    void pop_back();
    void clear();

    // Scrolling
    virtual void scroll_up(std::size_t n = 1);
    virtual void scroll_down(std::size_t n = 1);

    // Word Wrapping
    void enable_word_wrap(bool enable = true);
    void disable_word_wrap(bool disable = true);
    void toggle_word_wrap();

    // Incoming Text Attributes
    void add_new_text_attribute(Attribute attr);
    void remove_new_text_attribute(Attribute attr);
    void clear_new_text_attributes();

    // Query Functions
    std::size_t row_length(std::size_t y) const;
    std::size_t display_height() const;
    std::size_t index_at(Coordinates position) const;
    std::size_t index_at(std::size_t x, std::size_t y) const;
    Coordinates display_position(std::size_t index) const;
    Glyph_string contents() const { return contents_; }
    Glyph glyph_at(std::size_t index) const { return contents_.at(index); }
    std::size_t contents_size() const { return contents_.size(); }
    bool contents_empty() const { return contents_.empty(); }
    bool does_word_wrap() const { return word_wrap_; }

    // Signals
    sig::Signal<void(std::size_t n)> scrolled_up;
    sig::Signal<void(std::size_t n)> scrolled_down;
    sig::Signal<void()> scrolled;
    sig::Signal<void(const Glyph_string&)> text_changed;

   protected:
    bool paint_event() override;
    bool resize_event(std::size_t new_width,
                      std::size_t new_height,
                      std::size_t old_width,
                      std::size_t old_height) override;

    std::size_t line_at(std::size_t index) const;
    std::size_t top_line() const;
    std::size_t bottom_line() const;
    std::size_t last_line() const;

    std::size_t first_index_at(std::size_t line) const;
    std::size_t last_index_at(std::size_t line) const;

    std::size_t line_length(std::size_t line) const;
    std::size_t end_index() const;

   private:
    struct line_info {
        std::size_t start_index;
        std::size_t length;
    };

    std::vector<line_info> display_state_{line_info{0, 0}};

    std::size_t top_line_{0};
    bool word_wrap_ = true;
    Glyph_string contents_;
    Brush new_text_brush_{this->brush};  // TODO possibly make public member

    void update_display(std::size_t from_line = 0);
};

}  // namespace cppurses
#endif  // WIDGET_WIDGETS_TEXT_DISPLAY_HPP