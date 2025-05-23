/*
 * Copyright (c) 2022, Andreas Kling <andreas@ladybird.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/Bindings/Intrinsics.h>
#include <LibWeb/Bindings/MediaQueryListEventPrototype.h>
#include <LibWeb/CSS/MediaQueryListEvent.h>

namespace Web::CSS {

GC_DEFINE_ALLOCATOR(MediaQueryListEvent);

GC::Ref<MediaQueryListEvent> MediaQueryListEvent::create(JS::Realm& realm, FlyString const& event_name, MediaQueryListEventInit const& event_init)
{
    auto event = realm.create<MediaQueryListEvent>(realm, event_name, event_init);
    event->set_is_trusted(true);
    return event;
}

GC::Ref<MediaQueryListEvent> MediaQueryListEvent::construct_impl(JS::Realm& realm, FlyString const& event_name, MediaQueryListEventInit const& event_init)
{
    return realm.create<MediaQueryListEvent>(realm, event_name, event_init);
}

MediaQueryListEvent::MediaQueryListEvent(JS::Realm& realm, FlyString const& event_name, MediaQueryListEventInit const& event_init)
    : DOM::Event(realm, event_name, event_init)
    , m_media(event_init.media)
    , m_matches(event_init.matches)
{
}

MediaQueryListEvent::~MediaQueryListEvent() = default;

void MediaQueryListEvent::initialize(JS::Realm& realm)
{
    WEB_SET_PROTOTYPE_FOR_INTERFACE(MediaQueryListEvent);
    Base::initialize(realm);
}

}
