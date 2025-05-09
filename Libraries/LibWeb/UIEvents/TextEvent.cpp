/*
 * Copyright (c) 2024, Jamie Mansfield <jmansfield@cadixdev.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/Bindings/Intrinsics.h>
#include <LibWeb/Bindings/TextEventPrototype.h>
#include <LibWeb/UIEvents/TextEvent.h>

namespace Web::UIEvents {

GC_DEFINE_ALLOCATOR(TextEvent);

GC::Ref<TextEvent> TextEvent::create(JS::Realm& realm, FlyString const& event_name)
{
    return realm.create<TextEvent>(realm, event_name);
}

TextEvent::TextEvent(JS::Realm& realm, FlyString const& event_name)
    : UIEvent(realm, event_name)
{
}

TextEvent::~TextEvent() = default;

void TextEvent::initialize(JS::Realm& realm)
{
    WEB_SET_PROTOTYPE_FOR_INTERFACE(TextEvent);
    Base::initialize(realm);
}

// https://w3c.github.io/uievents/#dom-textevent-inittextevent
void TextEvent::init_text_event(String const& type, bool bubbles, bool cancelable, GC::Ptr<HTML::WindowProxy> view, String const& data)
{
    // Initializes attributes of a TextEvent object. This method has the same behavior as UIEvent.initUIEvent().
    // The value of detail remains undefined.

    // 1. If this’s dispatch flag is set, then return.
    if (dispatched())
        return;

    // 2. Initialize this with type, bubbles, and cancelable.
    initialize_event(type, bubbles, cancelable);

    // Implementation Defined: Initialise other values.
    m_view = view;
    m_data = data;
}

}
