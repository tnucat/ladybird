/*
 * Copyright (c) 2023, MacDue <macdue@dueutil.tech>
 * Copyright (c) 2024, Aliaksandr Kalenik <kalenik.aliaksandr@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibGfx/Bitmap.h>
#include <LibWeb/Bindings/CanvasPatternPrototype.h>
#include <LibWeb/Bindings/Intrinsics.h>
#include <LibWeb/HTML/CanvasPattern.h>
#include <LibWeb/HTML/CanvasRenderingContext2D.h>
#include <LibWeb/HTML/ImageBitmap.h>
#include <LibWeb/SVG/SVGImageElement.h>

namespace Web::HTML {

GC_DEFINE_ALLOCATOR(CanvasPattern);

// https://html.spec.whatwg.org/multipage/canvas.html#dom-canvaspattern-settransform
void CanvasPatternPaintStyle::paint(Gfx::IntRect physical_bounding_box, PaintFunction paint) const
{
    // 1. Create an infinite transparent black bitmap.
    // *waves magic wand 🪄*
    // Done!

    // 2. Place a copy of the image on the bitmap, anchored such that its top left corner
    // is at the origin of the coordinate space, with one coordinate space unit per CSS pixel of the image,
    // then place repeated copies of this image horizontally to the left and right, if the repetition behavior
    // is "repeat-x", or vertically up and down, if the repetition behavior is "repeat-y", or in all four directions
    // all over the bitmap, if the repetition behavior is "repeat".

    // FIXME: If the original image data is a bitmap image, then the value painted at a point in the area of
    // the repetitions is computed by filtering the original image data. When scaling up, if the imageSmoothingEnabled
    // attribute is set to false, then the image must be rendered using nearest-neighbor interpolation.
    // Otherwise, the user agent may use any filtering algorithm (for example bilinear interpolation or nearest-neighbor).
    // User agents which support multiple filtering algorithms may use the value of the imageSmoothingQuality attribute
    // to guide the choice of filtering algorithm. When such a filtering algorithm requires a pixel value from outside
    // the original image data, it must instead use the value from wrapping the pixel's coordinates to the original
    // image's dimensions. (That is, the filter uses 'repeat' behavior, regardless of the value of the pattern's repetition behavior.)

    // FIXME: 3. Transform the resulting bitmap according to the pattern's transformation matrix.

    // FIXME: 4. Transform the resulting bitmap again, this time according to the current transformation matrix.

    // 5. Replace any part of the image outside the area in which the pattern is to be rendered with transparent black.

    // 6. The resulting bitmap is what is to be rendered, with the same origin and same scale.

    // FIXME: This doesn't handle a 'none' canvas context mode.
    auto bitmap = m_image.visit(
        [](GC::Root<HTMLImageElement> const& source) -> RefPtr<Gfx::ImmutableBitmap> { return source->immutable_bitmap(); },
        [](GC::Root<SVG::SVGImageElement> const& source) -> RefPtr<Gfx::ImmutableBitmap> { return source->current_image_bitmap(); },
        [](GC::Root<OffscreenCanvas> const& source) -> RefPtr<Gfx::ImmutableBitmap> { return Gfx::ImmutableBitmap::create(*source->bitmap()); },
        [](GC::Root<HTMLCanvasElement> const& source) -> RefPtr<Gfx::ImmutableBitmap> { return Gfx::ImmutableBitmap::create_snapshot_from_painting_surface(*source->surface()); },
        [](GC::Root<HTMLVideoElement> const& source) -> RefPtr<Gfx::ImmutableBitmap> { return Gfx::ImmutableBitmap::create(*source->bitmap()); },
        [](GC::Root<ImageBitmap> const& source) -> RefPtr<Gfx::ImmutableBitmap> { return Gfx::ImmutableBitmap::create(*source->bitmap()); });
    VERIFY(bitmap);

    auto const bitmap_width = bitmap->width();
    auto const bitmap_height = bitmap->height();

    paint([=, this](auto point) {
        point.translate_by(physical_bounding_box.location());
        point = [&]() -> Gfx::IntPoint {
            switch (m_repetition) {
            case Repetition::NoRepeat: {
                return point;
            }
            case Repetition::Repeat: {
                return {
                    point.x() % bitmap_width,
                    point.y() % bitmap_height
                };
            }
            case Repetition::RepeatX: {
                return {
                    point.x() % bitmap_width,
                    point.y()
                };
            }
            case Repetition::RepeatY: {
                return {
                    point.x(),
                    point.y() % bitmap_height
                };
            }
            default:
                VERIFY_NOT_REACHED();
            }
        }();
        if (bitmap->rect().contains(point))
            return bitmap->get_pixel(point.x(), point.y());
        return Gfx::Color();
    });
}

CanvasPattern::CanvasPattern(JS::Realm& realm, CanvasPatternPaintStyle& pattern)
    : PlatformObject(realm)
    , m_pattern(pattern)
{
}

CanvasPattern::~CanvasPattern() = default;

// https://html.spec.whatwg.org/multipage/canvas.html#dom-context-2d-createpattern
WebIDL::ExceptionOr<GC::Ptr<CanvasPattern>> CanvasPattern::create(JS::Realm& realm, CanvasImageSource const& image, StringView repetition)
{
    auto parse_repetition = [&](auto repetition) -> Optional<CanvasPatternPaintStyle::Repetition> {
        if (repetition == "repeat"sv)
            return CanvasPatternPaintStyle::Repetition::Repeat;
        if (repetition == "repeat-x"sv)
            return CanvasPatternPaintStyle::Repetition::RepeatX;
        if (repetition == "repeat-y"sv)
            return CanvasPatternPaintStyle::Repetition::RepeatY;
        if (repetition == "no-repeat"sv)
            return CanvasPatternPaintStyle::Repetition::NoRepeat;
        return {};
    };

    // 1. Let usability be the result of checking the usability of image.
    auto usability = TRY(check_usability_of_image(image));

    // 2. If usability is bad, then return null.
    if (usability == CanvasImageSourceUsability::Bad)
        return GC::Ptr<CanvasPattern> {};

    // 3. Assert: usability is good.
    VERIFY(usability == CanvasImageSourceUsability::Good);

    // 4. If repetition is the empty string, then set it to "repeat".
    if (repetition.is_empty())
        repetition = "repeat"sv;

    // 5. If repetition is not identical to one of "repeat", "repeat-x", "repeat-y", or "no-repeat",
    // then throw a "SyntaxError" DOMException.
    auto repetition_value = parse_repetition(repetition);
    if (!repetition_value.has_value())
        return WebIDL::SyntaxError::create(realm, "Repetition value is not valid"_string);

    // 6. Let pattern be a new CanvasPattern object with the image image and the repetition behavior given by repetition.
    auto pattern = TRY_OR_THROW_OOM(realm.vm(), CanvasPatternPaintStyle::create(image, *repetition_value));

    // FIXME: 7. If image is not origin-clean, then mark pattern as not origin-clean.

    // 8. Return pattern.
    return realm.create<CanvasPattern>(realm, *pattern);
}

void CanvasPattern::initialize(JS::Realm& realm)
{
    WEB_SET_PROTOTYPE_FOR_INTERFACE(CanvasPattern);
    Base::initialize(realm);
}

}
