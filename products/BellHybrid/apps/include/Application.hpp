// Copyright (c) 2017-2021, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#pragma once

#include <ApplicationCommon.hpp>

namespace app
{
    class Application : public ApplicationCommon
    {
      public:
        using ApplicationCommon::ApplicationCommon;
        void resumeIdleTimer();
        void suspendIdleTimer();
        explicit Application(std::string name,
                             std::string parent                  = "",
                             StatusIndicators statusIndicators   = StatusIndicators{},
                             StartInBackground startInBackground = {false},
                             uint32_t stackDepth                 = 4096,
                             sys::ServicePriority priority       = sys::ServicePriority::Idle);

      protected:
        void attachPopups(const std::vector<gui::popup::ID> &popupsList) override;
        void startIdleTimer();
        void restartIdleTimer();
        void stopIdleTimer();
        void stopAllAudio();
        virtual void onStop();

      private:
        sys::MessagePointer handleKBDKeyEvent(sys::Message *msgl) override;
        sys::MessagePointer handleApplicationSwitch(sys::Message *msgl) override;
        sys::MessagePointer handleAppClose(sys::Message *msgl) override;
        sys::MessagePointer handleAppFocusLost(sys::Message *msgl) override;
        void updateStatuses(gui::AppWindow *window) const override;

        virtual void onKeyPressed();
        virtual void onStart();
        bool idleTimerActiveFlag = true;
    };
} // namespace app
