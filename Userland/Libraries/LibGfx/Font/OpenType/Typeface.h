/*
 * Copyright (c) 2020, Srimanta Barua <srimanta.barua1@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Noncopyable.h>
#include <AK/NonnullOwnPtr.h>
#include <AK/OwnPtr.h>
#include <AK/RefCounted.h>
#include <AK/StringView.h>
#include <LibCore/Resource.h>
#include <LibGfx/Bitmap.h>
#include <LibGfx/Font/Font.h>
#include <LibGfx/Font/OpenType/Cmap.h>
#include <LibGfx/Font/OpenType/Glyf.h>
#include <LibGfx/Font/OpenType/Tables.h>
#include <LibGfx/Font/Typeface.h>

namespace OpenType {

class CharCodeToGlyphIndex {
public:
    virtual ~CharCodeToGlyphIndex() = default;
    virtual u32 glyph_id_for_code_point(u32) const = 0;
};

// This is not a nested struct to work around https://llvm.org/PR36684
struct FontOptions {
    unsigned index { 0 };
    OwnPtr<CharCodeToGlyphIndex> external_cmap {};

    enum SkipTables {
        // If set, do not try to read the 'name' table. family() and variant() will return empty strings.
        Name = 1 << 0,

        // If set, tolerate a missing or broken 'hmtx' table. This will make glyph_metrics() return 0 for everyting and is_fixed_width() return true.
        Hmtx = 1 << 1,

        // If set, tolerate a missing or broken 'OS/2' table. metrics(), resolve_ascender_and_descender(), weight(), width(), and slope() will return different values.
        OS2 = 1 << 2,
    };
    u32 skip_tables { 0 };
};

class Typeface : public Gfx::Typeface {
    AK_MAKE_NONCOPYABLE(Typeface);

public:
    using Options = FontOptions;
    static ErrorOr<NonnullRefPtr<Typeface>> try_load_from_resource(Core::Resource const&, unsigned index = 0);
    static ErrorOr<NonnullRefPtr<Typeface>> try_load_from_font_data(NonnullOwnPtr<Gfx::FontData>, Options options = {});
    static ErrorOr<NonnullRefPtr<Typeface>> try_load_from_externally_owned_memory(ReadonlyBytes bytes, Options options = {});

    virtual Gfx::ScaledFontMetrics metrics(float x_scale, float y_scale) const override;
    virtual Gfx::ScaledGlyphMetrics glyph_metrics(u32 glyph_id, float x_scale, float y_scale, float point_width, float point_height) const override;
    virtual float glyph_advance(u32 glyph_id, float x_scale, float y_scale, float point_width, float point_height) const override;
    virtual float glyphs_horizontal_kerning(u32 left_glyph_id, u32 right_glyph_id, float x_scale) const override;
    virtual u32 glyph_count() const override;
    virtual u16 units_per_em() const override;
    virtual u32 glyph_id_for_code_point(u32 code_point) const override;
    virtual String family() const override;
    virtual String variant() const override;
    virtual u16 weight() const override;
    virtual u16 width() const override;
    virtual u8 slope() const override;

    // https://learn.microsoft.com/en-us/typography/opentype/spec/otff
    // "OpenType fonts that contain TrueType outlines should use the value of 0x00010000 for the sfntVersion.
    //  OpenType fonts containing CFF data (version 1 or 2) should use 0x4F54544F ('OTTO', when re-interpreted as a Tag) for sfntVersion.
    //  Note: The Apple specification for TrueType fonts allows for 'true' and 'typ1' for sfnt version.
    //         These version tags should not be used for OpenType fonts."
    // "Font Collection ID string: 'ttcf' (used for fonts with CFF or CFF2 outlines as well as TrueType outlines)"
    // The old Apple TrueType spec said "Fonts with TrueType outlines produced for OS X or iOS only are encouraged to use 'true'",
    // so 'true' is somewhat common, especially in PDFs.
    static constexpr Tag HeaderTag_TrueTypeOutlines = Tag::from_u32(0x00010000);
    static constexpr Tag HeaderTag_TrueTypeOutlinesApple = Tag { "true" };
    static constexpr Tag HeaderTag_CFFOutlines = Tag { "OTTO" };
    static constexpr Tag HeaderTag_FontCollection = Tag { "ttcf" };

protected:
    virtual ReadonlyBytes buffer() const override { return m_buffer; }
    virtual unsigned ttc_index() const override { return m_ttc_index; }

private:
    static ErrorOr<NonnullRefPtr<Typeface>> try_load_from_offset(ReadonlyBytes, u32 offset, Options options);

    Typeface(
        Head&& head,
        Optional<Name>&& name,
        Hhea&& hhea,
        Maxp&& maxp,
        Optional<Hmtx>&& hmtx,
        NonnullOwnPtr<CharCodeToGlyphIndex> cmap,
        Optional<Loca>&& loca,
        Optional<Glyf>&& glyf,
        Optional<OS2> os2,
        Optional<Kern>&& kern,
        Optional<GPOS> gpos,
        ReadonlyBytes buffer,
        unsigned ttc_index)
        : m_buffer(buffer)
        , m_ttc_index(ttc_index)
        , m_head(move(head))
        , m_name(move(name))
        , m_hhea(move(hhea))
        , m_maxp(move(maxp))
        , m_hmtx(move(hmtx))
        , m_loca(move(loca))
        , m_glyf(move(glyf))
        , m_cmap(move(cmap))
        , m_os2(move(os2))
        , m_kern(move(kern))
        , m_gpos(move(gpos))
    {
    }

    OwnPtr<Gfx::FontData> m_font_data;
    ReadonlyBytes m_buffer;
    unsigned m_ttc_index { 0 };

    // These are stateful wrappers around non-owning slices
    Head m_head;
    Optional<Name> m_name;
    Hhea m_hhea;
    Maxp m_maxp;
    Optional<Hmtx> m_hmtx;
    Optional<Loca> m_loca;
    Optional<Glyf> m_glyf;
    NonnullOwnPtr<CharCodeToGlyphIndex> m_cmap;
    Optional<OS2> m_os2;
    Optional<Kern> m_kern;
    Optional<GPOS> m_gpos;

    // This cache stores information per code point.
    // It's segmented into pages with data about 256 code points each.
    struct GlyphPage {
        static constexpr size_t glyphs_per_page = 256;

        u32 glyph_ids[glyphs_per_page];
    };

    // Fast cache for GlyphPage #0 (code points 0-255) to avoid hash lookups for all of ASCII and Latin-1.
    OwnPtr<GlyphPage> mutable m_glyph_page_zero;

    HashMap<size_t, NonnullOwnPtr<GlyphPage>> mutable m_glyph_pages;

    HashMap<u32, i16> mutable m_kerning_cache;

    Optional<String> mutable m_family;
    Optional<u16> mutable m_width;
    Optional<u16> mutable m_weight;
    Optional<u8> mutable m_slope;

    GlyphPage const& glyph_page(size_t page_index) const;
    void populate_glyph_page(GlyphPage&, size_t page_index) const;
};

}
