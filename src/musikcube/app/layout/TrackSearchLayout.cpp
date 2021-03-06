//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2007-2017 musikcube team
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the author nor the names of other contributors may
//      be used to endorse or promote products derived from this software
//      without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <cursespp/Colors.h>
#include <cursespp/Screen.h>
#include <core/library/LocalLibraryConstants.h>
#include <core/library/query/local/SearchTrackListQuery.h>
#include <app/util/Hotkeys.h>
#include <app/util/Messages.h>
#include <app/util/Playback.h>
#include <app/overlay/PlayQueueOverlays.h>

#include "TrackSearchLayout.h"

using namespace musik::core;
using namespace musik::core::audio;
using namespace musik::core::db::local;
using namespace musik::core::library;
using namespace musik::core::library::constants;
using namespace musik::core::runtime;
using namespace musik::cube;
using namespace cursespp;

#define SEARCH_HEIGHT 3
#define REQUERY_INTERVAL_MS 300

TrackSearchLayout::TrackSearchLayout(
    musik::core::audio::PlaybackService& playback,
    musik::core::ILibraryPtr library)
: LayoutBase()
, playback(playback)
, library(library) {
    this->InitializeWindows();
}

TrackSearchLayout::~TrackSearchLayout() {
}

void TrackSearchLayout::OnLayout() {
    size_t cx = this->GetWidth(), cy = this->GetHeight();
    int x = 0, y = 0;

    size_t inputWidth = cx / 2;
    size_t inputX = x + ((cx - inputWidth) / 2);
    this->input->MoveAndResize(inputX, y, cx / 2, SEARCH_HEIGHT);

    this->trackList->MoveAndResize(
        x,
        y + SEARCH_HEIGHT,
        this->GetWidth(),
        this->GetHeight() - SEARCH_HEIGHT);

    this->trackList->SetFrameTitle(_TSTR("track_filter_title"));
}

void TrackSearchLayout::InitializeWindows() {
    this->input.reset(new cursespp::TextInput());
    this->input->TextChanged.connect(this, &TrackSearchLayout::OnInputChanged);
    this->input->EnterPressed.connect(this, &TrackSearchLayout::OnEnterPressed);
    this->input->SetFocusOrder(0);
    this->AddWindow(this->input);

    this->trackList.reset(new TrackListView(this->playback, this->library));
    this->trackList->SetFocusOrder(1);
    this->trackList->SetAllowArrowKeyPropagation();
    this->AddWindow(this->trackList);
}

void TrackSearchLayout::OnVisibilityChanged(bool visible) {
    LayoutBase::OnVisibilityChanged(visible);

    if (visible) {
        this->SetFocus(this->input);
        this->Requery();
    }
    else {
        this->input->SetText("");
        this->trackList->Clear();
    }
}

void TrackSearchLayout::FocusInput() {
    this->SetFocus(this->input);
}

void TrackSearchLayout::Requery() {
    const std::string& filter = this->input->GetText();
    this->trackList->Requery(std::shared_ptr<TrackListQueryBase>(
        new SearchTrackListQuery(this->library, filter)));
}

void TrackSearchLayout::ProcessMessage(IMessage &message) {
    int type = message.Type();

    if (type == message::RequeryTrackList) {
        this->Requery();
    }
}

void TrackSearchLayout::OnInputChanged(cursespp::TextInput* sender, std::string value) {
    if (this->IsVisible()) {
        DebounceMessage(message::RequeryTrackList, 0, 0, REQUERY_INTERVAL_MS);
    }
}

void TrackSearchLayout::OnEnterPressed(cursespp::TextInput* sender) {
    if (this->trackList->GetTrackList()->Count()) {
        playback::Play(*(this->trackList.get()), this->playback);
        this->SetFocus(this->trackList);
    }
}

bool TrackSearchLayout::KeyPress(const std::string& key) {
    if (Hotkeys::Is(Hotkeys::Down, key)) {
        if (this->GetFocus() == this->input) {
            this->FocusNext();
            return true;
        }
    }
    else if (Hotkeys::Is(Hotkeys::Up, key)) {
        if (this->GetFocus() == this->trackList) {
            this->SetFocus(this->input);
            return true;
        }
    }

    return LayoutBase::KeyPress(key);
}
