// Liquid-glass style panels for the chat header and the message field.
//
// The panel paints a blurred, downscaled copy of the chat wallpaper that
// lies under it, then a translucent tint, a soft top highlight and a thin
// light border, which together read as frosted glass.
#pragma once

namespace Ui {
class ChatTheme;
} // namespace Ui

namespace style {
struct InputField;
} // namespace style

namespace Window {
class SessionController;
} // namespace Window

namespace AyuGlass {

struct Shape {
	int radius = 0; // 0 means a plain rectangle (header, edge-to-edge).
	bool border = false; // Thin light outline, used for floating capsules.
	bool topHighlight = true; // Bright 1px line along the top edge.
};

// Is the glass effect turned on in the settings.
[[nodiscard]] bool Enabled();

// Should the message field be drawn as a floating rounded capsule.
[[nodiscard]] bool FloatingInput();

// Corner radius of the floating capsule for a panel of this height.
[[nodiscard]] int CapsuleRadius(int height);

// Gap between the floating capsule and the edges of the chat.
[[nodiscard]] int CapsuleInset();

// Style for the message field: with glass on, the field itself is
// transparent so the glass under it shows through. Read once at startup.
[[nodiscard]] const style::InputField &ComposeFieldStyle();

// Background colour for the fade at the edges of the message field.
[[nodiscard]] const style::color &ComposeFieldFadeBg();

// Paints a glass panel into `target` (in the painter's coordinates).
//
// `bgWidget` is the widget that paints the chat wallpaper (the chat section
// itself), and `originInBg` is where target.topLeft() lies in its
// coordinates, so the blurred wallpaper lines up with the real one.
void PaintChatGlass(
	QPainter &p,
	QRect target,
	not_null<Window::SessionController*> controller,
	not_null<Ui::ChatTheme*> theme,
	not_null<QWidget*> bgWidget,
	QPoint originInBg,
	Shape shape = {});

} // namespace AyuGlass
