#include "ayu/ui/glass/glass_effect.h"

#include "ayu/ayu_settings.h"
#include "mainwidget.h"
#include "ui/chat/chat_theme.h"
#include "ui/image/image_prepare.h"
#include "ui/painter.h"
#include "window/section_widget.h"
#include "window/window_session_controller.h"
#include "styles/style_basic.h"
#include "styles/style_chat_helpers.h"

#include <QtGui/QPainterPath>
#include <QtGui/QLinearGradient>

namespace AyuGlass {
namespace {

// The wallpaper is rendered at 1/kDownscale of the real resolution before
// blurring. That keeps the blur cheap enough to run on every repaint and
// also makes it look smoother when scaled back up.
constexpr auto kDownscale = 4;

[[nodiscard]] bool IsDarkPalette() {
	return st::windowBg->c.lightness() < 128;
}

[[nodiscard]] QColor TintColor() {
	auto result = st::windowBg->c;
	const auto opacity = AyuSettings::getInstance().glassOpacity();
	result.setAlpha(std::clamp(opacity, 0, 100) * 255 / 100);
	return result;
}

[[nodiscard]] QImage BlurredWallpaper(
		QSize targetSize,
		not_null<Window::SessionController*> controller,
		not_null<Ui::ChatTheme*> theme,
		not_null<QWidget*> bgWidget,
		QPoint originInBg,
		int blurRadius,
		int padding) {
	const auto content = controller->content();
	const auto fromy = content->backgroundFromY();
	const auto fill = QSize(bgWidget->width(), content->height());
	const auto area = QRect(originInBg, targetSize).marginsAdded(
		{ padding, padding, padding, padding });

	const auto ratio = style::DevicePixelRatio();
	const auto scale = ratio / double(kDownscale);
	const auto size = QSize(
		std::max(1, int(std::ceil(area.width() * scale))),
		std::max(1, int(std::ceil(area.height() * scale))));

	auto image = QImage(size, QImage::Format_ARGB32_Premultiplied);
	image.fill(Qt::transparent);
	{
		auto q = QPainter(&image);
		q.setRenderHint(QPainter::SmoothPixmapTransform);
		q.scale(scale, scale);
		q.translate(-area.topLeft());
		q.translate(0, fromy);
		Window::SectionWidget::PaintBackground(
			q,
			theme,
			fill,
			area.translated(0, -fromy),
			controller->isGifPausedAtLeastFor(
				Window::GifPauseReason::Any));
	}
	return Images::BlurLargeImage(std::move(image), blurRadius);
}

} // namespace

bool Enabled() {
	return AyuSettings::getInstance().glassEnabled();
}

bool FloatingInput() {
	return Enabled() && AyuSettings::getInstance().glassFloatingInput();
}

const style::InputField &ComposeFieldStyle() {
	static const auto enabled = Enabled();
	if (!enabled) {
		return st::historyComposeField;
	}
	static const auto result = [] {
		auto copy = st::historyComposeField;
		copy.textBg = st::transparent;
		copy.textBgActive = st::transparent;
		return copy;
	}();
	return result;
}

const style::color &ComposeFieldFadeBg() {
	static const auto enabled = Enabled();
	return enabled ? st::transparent : st::historyComposeField.textBg;
}

int CapsuleRadius(int height) {
	return std::min(height / 2, style::ConvertScale(22));
}

int CapsuleInset() {
	return style::ConvertScale(6);
}

void PaintChatGlass(
		QPainter &p,
		QRect target,
		not_null<Window::SessionController*> controller,
		not_null<Ui::ChatTheme*> theme,
		not_null<QWidget*> bgWidget,
		QPoint originInBg,
		Shape shape) {
	if (target.isEmpty()) {
		return;
	}
	// Setting 1..10 maps to a blur radius in downscaled pixels.
	const auto blur = std::clamp(
		AyuSettings::getInstance().glassBlur(),
		1,
		10);
	// Extra wallpaper around the panel so the blur has real pixels to mix
	// at the edges instead of fading into transparency.
	const auto padding = (blur * kDownscale) / style::DevicePixelRatio() + 1;

	const auto image = BlurredWallpaper(
		target.size(),
		controller,
		theme,
		bgWidget,
		originInBg,
		blur,
		padding);

	const auto ratio = style::DevicePixelRatio();
	const auto scale = ratio / double(kDownscale);
	const auto source = QRectF(
		padding * scale,
		padding * scale,
		target.width() * scale,
		target.height() * scale);

	p.save();
	auto hq = PainterHighQualityEnabler(p);
	p.setRenderHint(QPainter::SmoothPixmapTransform);

	auto path = QPainterPath();
	if (shape.radius > 0) {
		path.addRoundedRect(QRectF(target), shape.radius, shape.radius);
		p.setClipPath(path, Qt::IntersectClip);
	}

	// 1. Frosted wallpaper.
	p.drawImage(QRectF(target), image, source);

	// 2. Tint, so text on top stays readable on any wallpaper.
	p.fillRect(target, TintColor());

	// 3. Soft light falling from the top edge, like on curved glass.
	const auto dark = IsDarkPalette();
	{
		auto gradient = QLinearGradient(
			QPointF(target.topLeft()),
			QPointF(target.left(), target.top() + target.height() * 0.6));
		gradient.setColorAt(0., QColor(255, 255, 255, dark ? 22 : 46));
		gradient.setColorAt(1., QColor(255, 255, 255, 0));
		p.fillRect(target, gradient);
	}

	// 4. Bright specular line along the top edge.
	if (shape.topHighlight && shape.radius == 0) {
		p.fillRect(
			QRect(target.x(), target.y(), target.width(), st::lineWidth),
			QColor(255, 255, 255, dark ? 30 : 90));
	}

	p.setClipping(false);

	// 5. Thin light outline for floating capsules.
	if (shape.border && shape.radius > 0) {
		const auto half = st::lineWidth / 2.;
		auto pen = QPen(QColor(255, 255, 255, dark ? 38 : 110));
		pen.setWidthF(st::lineWidth);
		p.setPen(pen);
		p.setBrush(Qt::NoBrush);
		p.drawRoundedRect(
			QRectF(target).marginsRemoved({ half, half, half, half }),
			shape.radius - half,
			shape.radius - half);
	}
	p.restore();
}

} // namespace AyuGlass
