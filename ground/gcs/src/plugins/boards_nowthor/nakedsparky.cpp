/**
 ******************************************************************************
 * @file       nakedsparky.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_NowthorPlugin Nowthor Corporation boards support Plugin
 * @{
 * @brief Plugin to support Nowthor Corporation boards
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "nakedsparky.h"

#include <uavobjectmanager.h>
#include "uavobjectutil/uavobjectutilmanager.h"
#include <extensionsystem/pluginmanager.h>

#include "hwnakedsparky.h"

/**
 * @brief NakedSparky::NakedSparky
 *  This is the NakedSparky board definition
 */
NakedSparky::NakedSparky(void)
{
    // Initialize our USB Structure definition here:
    USBInfo board;
    board.vendorID = 0x16d0;
    board.productID = 0x0CBB;

    setUSBInfo(board);

    boardType = 0xAA;

    // Define the bank of channels that are connected to a given timer
    channelBanks.resize(6);
    channelBanks[0] = QVector<int> () << 1 << 2;      // TIM15
    channelBanks[1] = QVector<int> () << 3;           // TIM1
    channelBanks[2] = QVector<int> () << 4 << 7 << 9; // TIM3
    channelBanks[3] = QVector<int> () << 5;           // TIM16
    channelBanks[4] = QVector<int> () << 6 << 10;     // TIM2
    channelBanks[5] = QVector<int> () << 8;           // TIM17
}

NakedSparky::~NakedSparky()
{

}


QString NakedSparky::shortName()
{
    return QString("NakedSparky");
}

QString NakedSparky::boardDescription()
{
    return QString("NakedSparky Flight Controller");
}

//! Return which capabilities this board has
bool NakedSparky::queryCapabilities(BoardCapabilities capability)
{
    switch(capability) {
    case BOARD_CAPABILITIES_GYROS:
    case BOARD_CAPABILITIES_ACCELS:
    case BOARD_CAPABILITIES_MAGS:
    case BOARD_CAPABILITIES_BAROS:
    case BOARD_CAPABILITIES_UPGRADEABLE:
        return true;
    default:
        return false;
    }
    return false;
}

QPixmap NakedSparky::getBoardPicture()
{
    return QPixmap(":/nowthor/images/nakedsparky.png");
}

QString NakedSparky::getHwUAVO()
{
    return "HwNakedSparky";
}

//! Determine if this board supports configuring the receiver
bool NakedSparky::isInputConfigurationSupported(enum InputType type = INPUT_TYPE_ANY)
{
    switch (type) {
    case INPUT_TYPE_PWM:
    case INPUT_TYPE_HOTTSUMH:
        return false;
    default:
        return true;
    }
}

/**
 * Configure the board to use a receiver input type on a port number
 * @param type the type of receiver to use
 * @return true if successfully configured or false otherwise
 */
bool NakedSparky::setInputType(enum InputType type)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwNakedSparky *hwNakedSparky = HwNakedSparky::GetInstance(uavoManager);
    Q_ASSERT(hwNakedSparky);
    if (!hwNakedSparky)
        return false;

    HwNakedSparky::DataFields settings = hwNakedSparky->getData();

    switch(type) {
    case INPUT_TYPE_PPM:
        settings.RcvrPort = HwNakedSparky::RCVRPORT_PPM;
        break;
    case INPUT_TYPE_SBUS:
	    settings.RcvrPort = HwNakedSparky::RCVRPORT_SBUS;
        break;
	case INPUT_TYPE_SBUSNONINVERTED:
		settings.RcvrPort = HwNakedSparky::RCVRPORT_SBUSNONINVERTED;
		break;
    case INPUT_TYPE_DSM:
        settings.RcvrPort = HwNakedSparky::RCVRPORT_DSM;
        break;
    case INPUT_TYPE_HOTTSUMD:
        settings.RcvrPort = HwNakedSparky::RCVRPORT_HOTTSUMD;
        break;
    default:
        return false;
    }

    // Apply these changes
    hwNakedSparky->setData(settings);

    return true;
}

/**
 * @brief NakedSparky::getInputType fetch the currently selected input type
 * @return the selected input type
 */
enum Core::IBoardType::InputType NakedSparky::getInputType()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwNakedSparky *hwNakedSparky = HwNakedSparky::GetInstance(uavoManager);
    Q_ASSERT(hwNakedSparky);
    if (!hwNakedSparky)
        return INPUT_TYPE_UNKNOWN;

    HwNakedSparky::DataFields settings = hwNakedSparky->getData();

    switch(settings.RcvrPort) {
    case HwNakedSparky::RCVRPORT_PPM:
        return INPUT_TYPE_PPM;
    case HwNakedSparky::RCVRPORT_SBUS:
    	return INPUT_TYPE_SBUS;
    case HwNakedSparky::RCVRPORT_SBUSNONINVERTED:
	return INPUT_TYPE_SBUSNONINVERTED;
    case HwNakedSparky::RCVRPORT_DSM:
        return INPUT_TYPE_DSM;
    case HwNakedSparky::RCVRPORT_HOTTSUMD:
        return INPUT_TYPE_HOTTSUMD;
    default:
        return INPUT_TYPE_UNKNOWN;
    }
}

int NakedSparky::queryMaxGyroRate()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwNakedSparky *hwNakedSparky = HwNakedSparky::GetInstance(uavoManager);
    Q_ASSERT(hwNakedSparky);
    if (!hwNakedSparky)
        return 0;

    HwNakedSparky::DataFields settings = hwNakedSparky->getData();

    switch(settings.GyroRange) {
    case HwNakedSparky::GYRORANGE_250:
        return 250;
    case HwNakedSparky::GYRORANGE_500:
        return 500;
    case HwNakedSparky::GYRORANGE_1000:
        return 1000;
    case HwNakedSparky::GYRORANGE_2000:
        return 2000;
    default:
        return 500;
    }
}

QStringList NakedSparky::getAdcNames()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *uavoManager = pm->getObject<UAVObjectManager>();
    HwNakedSparky *hwNakedSparky = HwNakedSparky::GetInstance(uavoManager);
    Q_ASSERT(hwNakedSparky);
    if (!hwNakedSparky)
        return QStringList();

    QStringList names;
    HwNakedSparky::DataFields settings = hwNakedSparky->getData();
    if (settings.AuxPort == HwNakedSparky::AUXPORT_PWM82ADC || settings.AuxPort == HwNakedSparky::AUXPORT_PWM72ADCPWM_IN)
        names << "Battery" << "PWM10" << "PWM9" << "Disabled";
    else if (settings.AuxPort == HwNakedSparky::AUXPORT_PWM73ADC)
        names << "Battery" << "PWM10" << "PWM9" << "PWM8";
    else
        names << "Battery" << "Disabled" << "Disabled" << "Disabled";

    return names;
}
