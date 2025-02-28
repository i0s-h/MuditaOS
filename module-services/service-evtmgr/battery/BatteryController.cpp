// Copyright (c) 2017-2022, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#include "BatteryController.hpp"
#include "system/Constants.hpp"

#if ENABLE_CURRENT_MEASUREMENT_SCOPE
#include "CurrentMeasurementScope.hpp"
#endif

#include <service-evtmgr/BatteryMessages.hpp>
#include <service-evtmgr/Constants.hpp>
#include <module-utils/EventStore/EventStore.hpp>
#include <log/log.hpp>
#include <magic_enum.hpp>

using sevm::battery::BatteryController;

namespace
{
    using hal::battery::AbstractBatteryCharger;
    Store::Battery::State transformChargingState(AbstractBatteryCharger::ChargingStatus status)
    {
        using Status   = AbstractBatteryCharger::ChargingStatus;
        using NewState = Store::Battery::State;
        switch (status) {
        case Status::Discharging:
            return NewState::Discharging;
        case Status::Charging:
            return NewState::Charging;
        case Status::ChargingDone:
            return NewState ::ChargingDone;
        case Status::PluggedNotCharging:
            return NewState ::PluggedNotCharging;
        default:
            return NewState::Discharging;
        }
    }

    BatteryState::ChargingState transformChargingState(Store::Battery::State status)
    {
        using Status   = Store::Battery::State;
        using NewState = BatteryState::ChargingState;
        switch (status) {
        case Status::Discharging:
            return NewState::Discharging;
        case Status::Charging:
            return NewState::Charging;
        case Status::ChargingDone:
            return NewState ::ChargingDone;
        case Status::PluggedNotCharging:
            return NewState ::PluggedNotCharging;
        default:
            return NewState::Discharging;
        }
    }

    Store::Battery::LevelState transformBatteryState(BatteryState::State status)
    {
        using Status   = BatteryState::State;
        using NewState = Store::Battery::LevelState;
        switch (status) {
        case Status::Normal:
            return NewState::Normal;
        case Status::Shutdown:
            return NewState::Shutdown;
        case Status::CriticalCharging:
            return NewState ::CriticalCharging;
        case Status::CriticalNotCharging:
            return NewState ::CriticalNotCharging;
        default:
            return NewState::Normal;
        }
    }
} // namespace

BatteryController::BatteryController(sys::Service *service, xQueueHandle notificationChannel)
    : service{service}, charger{hal::battery::AbstractBatteryCharger::Factory::create(notificationChannel)},
      brownoutDetector(service, *charger),
      batteryState{service, [this](const auto state) {
                       Store::Battery::modify().levelState = transformBatteryState(state);
                       auto stateChangeMessage             = std::make_shared<sevm::BatteryStateChangeMessage>();
                       this->service->bus.sendUnicast(std::move(stateChangeMessage), service::name::system_manager);
                   }}
{
    Store::Battery::modify().level = charger->getSOC();
    Store::Battery::modify().state = transformChargingState(charger->getChargingStatus());
    batteryState.check(transformChargingState(Store::Battery::modify().state), Store::Battery::modify().level);

    LOG_INFO("Initial charger state:%s", magic_enum::enum_name(Store::Battery::get().state).data());
    LOG_INFO("Initial battery SOC:%d", Store::Battery::get().level);
    LOG_INFO("Initial battery voltage:%" PRIu32 "mV", charger->getBatteryVoltage());
    LOG_INFO("Initial battery state:%s", magic_enum::enum_name(Store::Battery::get().levelState).data());
}

void sevm::battery::BatteryController::handleNotification(Events evt)
{
    LOG_INFO("Incoming event: %s", std::string{magic_enum::enum_name(evt)}.c_str());
    switch (evt) {
    case Events::Charger:
    case Events::SOC:
        update();
        break;
    case Events::Brownout:
        brownoutDetector.startDetection();
        break;
    }
}

void sevm::battery::BatteryController::poll()
{
    update();
}
void sevm::battery::BatteryController::printCurrentState()
{
    LOG_INFO("Charger state:%s Battery SOC %d voltage: %" PRIu32 "mV state: %s",
             magic_enum::enum_name(Store::Battery::get().state).data(),
             Store::Battery::get().level,
             charger->getBatteryVoltage(),
             magic_enum::enum_name(Store::Battery::get().levelState).data());
}
void sevm::battery::BatteryController::update()
{
    const auto soc           = Store::Battery::get().level;
    const auto chargingState = Store::Battery::get().state;

    Store::Battery::modify().level = charger->getSOC();
    Store::Battery::modify().state = transformChargingState(charger->getChargingStatus());

    /// Send BatteryStatusChangeMessage only when battery SOC or charger state has changed
    if (soc != Store::Battery::get().level || chargingState != Store::Battery::get().state) {
        auto message = std::make_shared<sevm::BatteryStatusChangeMessage>();
        service->bus.sendUnicast(std::move(message), service::name::evt_manager);
    }

    batteryState.check(transformChargingState(Store::Battery::modify().state), Store::Battery::modify().level);

    printCurrentState();
}
