#include <mcurses/system_module/detail/ncurses_event_dispatcher.hpp>
#include <mcurses/system_module/event.hpp>
#include <mcurses/system_module/events/mouse_event.hpp>
#include <mcurses/system_module/events/key_event.hpp>
#include <mcurses/system_module/events/resize_event.hpp>
#include <mcurses/system_module/system.hpp>
#include <mcurses/widget_module/widget.hpp>

#include <ncurses.h>

#include <memory>
#include <utility>

namespace mcurses {
namespace detail {

// take input from user
// decifer event
// find object in different ways depending on the event type
// post the event to the system queue
void NCurses_event_dispatcher::post_user_input()
{
	std::unique_ptr<Event> event = nullptr;
	Object* object = nullptr;

	int input = ::getch(); // blocking call

	std::pair<Object*, std::unique_ptr<Event>> obj_event_pair;

	switch(input)
	{
		case KEY_MOUSE:
			obj_event_pair = parse_mouse_event();
			object = obj_event_pair.first;
			event = std::move(obj_event_pair.second);
			break;

		case KEY_RESIZE:
			event = handle_resize_event();
			object = handle_resize_object();
			break;

		default: // Key_event
			event = handle_keyboard_event();
			object = handle_keyboard_object();
			if (static_cast<Key_event*>(event.get())->key() == 9) {
				System::cycle_tab_focus();
			}
			break;
	}

	if(object != nullptr && event != nullptr) {
		System::post_event(object, std::move(event));
	}

	return;
}

std::pair<Object*, std::unique_ptr<Event>>
NCurses_event_dispatcher::parse_mouse_event()
{
	::MEVENT mouse_event;
	if(::getmouse(&mouse_event) == OK) {
		// Find Object
		Object* object = find_object(mouse_event.x, mouse_event.y);

		// Parse NCurses Event
		Event::Type 			ev_type = Event::Type::None;
		Mouse_event::Button 	ev_button = Mouse_event::Button::NoButton;

		// Button 1 / Left Button
		if(mouse_event.bstate & BUTTON1_PRESSED) {
			ev_type = Event::Type::MouseButtonPress;
			ev_button = Mouse_event::Button::LeftButton;
		}
		else if(mouse_event.bstate & BUTTON1_RELEASED) {
			ev_type = Event::Type::MouseButtonRelease;
			ev_button = Mouse_event::Button::LeftButton;
		}

		// Button 2 / Middle Button
		else if(mouse_event.bstate & BUTTON2_PRESSED) {
			ev_type = Event::Type::MouseButtonPress;
			ev_button = Mouse_event::Button::MidButton;
		}
		else if(mouse_event.bstate & BUTTON2_RELEASED) {
			ev_type = Event::Type::MouseButtonRelease;
			ev_button = Mouse_event::Button::MidButton;
		}

		// Button 3 / Right Button
		else if(mouse_event.bstate & BUTTON3_PRESSED) {
			ev_type = Event::Type::MouseButtonPress;
			ev_button = Mouse_event::Button::RightButton;
		}
		else if(mouse_event.bstate & BUTTON3_RELEASED) {
			ev_type = Event::Type::MouseButtonRelease;
			ev_button = Mouse_event::Button::RightButton;
		}

		// Button 4 / Scroll Down?
		else if(mouse_event.bstate & BUTTON4_PRESSED) {
			ev_type = Event::Type::MouseButtonPress;
			ev_button = Mouse_event::Button::ScrollDown;
		}
		else if(mouse_event.bstate & BUTTON4_RELEASED) {
			ev_type = Event::Type::MouseButtonRelease;
			ev_button = Mouse_event::Button::ScrollDown;
		}

		// Button 5 / Scroll Up?
		else if(mouse_event.bstate & BUTTON5_PRESSED) {
			ev_type = Event::Type::MouseButtonPress;
			ev_button = Mouse_event::Button::ScrollUp;
		}
		else if(mouse_event.bstate & BUTTON5_RELEASED) {
			ev_type = Event::Type::MouseButtonRelease;
			ev_button = Mouse_event::Button::ScrollUp;
		}
		else {
			return std::pair<Object*, std::unique_ptr<Event>>{nullptr, nullptr};
		}

		Widget* widg = dynamic_cast<Widget*>(object);
		if(widg) {
			unsigned local_x = mouse_event.x - widg->global_x();
			unsigned local_y = mouse_event.y - widg->global_y();
			auto event = std::make_unique<Mouse_event>(ev_type, ev_button, mouse_event.x, mouse_event.y, local_x, local_y, mouse_event.id);
			return std::make_pair(object, std::move(event));
		}
	}
	return std::pair<Object*, std::unique_ptr<Event>>{nullptr, nullptr};
}

Object*
NCurses_event_dispatcher::find_object(unsigned x, unsigned y)
{
	Object* obj = System::head();
	if(obj == nullptr || !obj->has_coordinates(x,y)) { return nullptr; }

	bool keep_going = true;
	while(keep_going && !obj->children().empty()) {
		for(Object* child : obj->children()) {
			if(child->has_coordinates(x,y) && child->is_enabled()) {
				obj = child;
				keep_going = true;
				break;
			}
			keep_going = false;
		}
	}
	return obj;
}

std::unique_ptr<Event>
NCurses_event_dispatcher::handle_keyboard_event()
{
	return std::unique_ptr<Event>{};
}

Object*
NCurses_event_dispatcher::handle_keyboard_object()
{
	return nullptr;
}

std::unique_ptr<Event>
NCurses_event_dispatcher::handle_resize_event() // sent to head widget and all children, etc.. sent to everything
{
	return std::unique_ptr<Event>{};
}

Object*
NCurses_event_dispatcher::handle_resize_object()
{
	return nullptr;
}











} // namespace detail
} // namespace mcurses