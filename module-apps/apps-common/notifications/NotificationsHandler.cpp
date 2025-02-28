﻿// Copyright (c) 2017-2022, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#include "NotificationsHandler.hpp"
#include <service-db/DBServiceAPI.hpp>
#include <log/log.hpp>
#include <service-cellular/service-cellular/CellularMessage.hpp>
#include <service-appmgr/Controller.hpp>
#include <service-audio/AudioServiceAPI.hpp>
#include <service-cellular/CellularServiceAPI.hpp>

using namespace notifications;

NotificationsHandler::NotificationsHandler(sys::Service *parentService, NotificationsConfiguration &notifcationConfig)
    : parentService{parentService}, notifcationConfig{notifcationConfig}
{}

void NotificationsHandler::registerMessageHandlers()
{
    parentService->connect(typeid(CellularIncomingSMSNotificationMessage),
                           [&](sys::Message *request) -> sys::MessagePointer {
                               incomingSMSHandler();
                               return sys::msgHandled();
                           });
}

void NotificationsHandler::incomingSMSHandler()
{
    notifcationConfig.updateCurrentSMS(currentSMSPolicy);
    playbackSMSRingtone();
}

void NotificationsHandler::playbackSMSRingtone()
{
    if (currentSMSPolicy.isRingtoneAllowed()) {
        const auto filePath = AudioServiceAPI::GetSound(parentService, audio::PlaybackType::TextMessageRingtone);
        AudioServiceAPI::PlaybackStart(parentService, audio::PlaybackType::TextMessageRingtone, filePath);
    }
}
