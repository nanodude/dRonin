/**
 ******************************************************************************
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup HITLPlugin HITL Plugin
 * @{
 *
 * @file       flightgearbridge.cpp
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief The Hardware In The Loop plugin
 *
 * @see        The GNU Public License (GPL) Version 3
 *
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include "fgsimulator.h"
#include "extensionsystem/pluginmanager.h"
#include "coreplugin/icore.h"

FGSimulator::FGSimulator(const SimulatorSettings& params) :
    Simulator(params)
{
    udpCounterFGrecv = 0;
    udpCounterGCSsend = 0;
}

FGSimulator::~FGSimulator()
{
    disconnect(simProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processReadyRead()));
}

void FGSimulator::setupUdpPorts(const QString& host, int inPort, int outPort)
{
    Q_UNUSED(outPort);

    if(inSocket->bind(QHostAddress(host), inPort))
        emit processOutput("Successfully bound to address " + host + " on port " + QString::number(inPort) + "\n");
    else
        emit processOutput("Cannot bind to address " + host + " on port " + QString::number(inPort) + "\n");
}

bool FGSimulator::setupProcess()
{
    // Copy FlightGear generic protocol configuration file to the FG protocol directory
    // NOTE: Not working on Windows 7, if FG is installed in the "Program Files",
    // likelly due to permissions. The file should be manually copied to data/Protocol/opfgprotocol.xml
    //	QFile xmlFile(":/flightgear/genericprotocol/opfgprotocol.xml");
    //	xmlFile.open(QIODevice::ReadOnly | QIODevice::Text);
    //	QString xml = xmlFile.readAll();
    //	xmlFile.close();
    //	QFile xmlFileOut(pathData + "/Protocol/opfgprotocol.xml");
    //	xmlFileOut.open(QIODevice::WriteOnly | QIODevice::Text);
    //	xmlFileOut.write(xml.toLatin1());
    //	xmlFileOut.close();

    Qt::HANDLE mainThread = QThread::currentThreadId();
    qDebug() << "setupProcess Thread: "<< mainThread;

    simProcess = new QProcess();
    simProcess->setReadChannelMode(QProcess::MergedChannels);
    connect(simProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processReadyRead()));
    // Note: Only tested on windows 7
#if defined(Q_WS_WIN)
    QString cmdShell("c:/windows/system32/cmd.exe");
#else
    QString cmdShell("bash");
#endif

    // Start shell (Note: Could not start FG directly on Windows, only through terminal!)
    simProcess->start(cmdShell);
    if (simProcess->waitForStarted() == false)
    {
        emit processOutput("Error:" + simProcess->errorString());
        return false;
    }

    // Setup arguments
    // Note: The input generic protocol is set to update at a much higher rate than the actual updates are sent by the GCS.
    // If this is not done then a lag will be introduced by FlightGear, likelly because the receive socket buffer builds up during startup.
    QString args("--fg-root=\"" + settings.dataPath + "\" " +
                 "--timeofday=noon " +
                 "--httpd=5400 " +
                 "--enable-hud " +
                 "--in-air " +
                 "--altitude=3000 " +
                 "--vc=100 " +
                 "--log-level=alert " +
                 "--generic=socket,out,20," + settings.hostAddress + "," + QString::number(settings.inPort) + ",udp,opfgprotocol");
    if(settings.manualControlEnabled) // <--[BCH] What does this do? Why does it depend on ManualControl?
    {
        args.append(" --generic=socket,in,400," + settings.remoteAddress + "," + QString::number(settings.outPort) + ",udp,opfgprotocol");
    }

    // Start FlightGear - only if checkbox is selected in HITL options page
    if (settings.startSim)
    {
        QString cmd("\"" + settings.binPath + "\" " + args + "\n");
        simProcess->write(cmd.toLatin1());
    }
    else
    {
        emit processOutput("Start Flightgear from the command line with the following arguments: \n\n" + args + "\n\n" +
                           "You can optionally run Flightgear from a networked computer.\n" +
                           "Make sure the computer running Flightgear can can ping your local interface adapter. ie." + settings.hostAddress + "\n"
                           "Remote computer must have the correct GCS protocol installed.");
    }

    udpCounterGCSsend = 0;

    return true;
}

void FGSimulator::processReadyRead()
{
    QByteArray bytes = simProcess->readAllStandardOutput();
    QString str(bytes);
    if ( !str.contains("Error reading data") ) // ignore error
    {
        emit processOutput(str);
    }
}

void FGSimulator::transmitUpdate()
{
    ActuatorDesired::DataFields actData;
    FlightStatus::DataFields flightStatusData = flightStatus->getData();
    float ailerons = -1;
    float elevator = -1;
    float rudder = -1;
    float throttle = -1;

    // Read ActuatorDesired from autopilot
    actData = actDesired->getData();

    // if we're in any mode other than manual, or if we're in manual and we're armed, update outputs from ActuatorDesired
    if(flightStatus->getFlightMode() != FlightStatus::FLIGHTMODE_MANUAL || flightStatusData.Armed == FlightStatus::ARMED_ARMED)
    {
        ailerons = actData.Roll;
        elevator = -actData.Pitch;
        rudder = actData.Yaw;
        throttle = actData.Thrust;
    }

    int allowableDifference = 10;

    //qDebug() << "UDP sent:" << udpCounterGCSsend << " - UDP Received:" << udpCounterFGrecv;

    if(udpCounterFGrecv == udpCounterGCSsend)
        udpCounterGCSsend = 0;
    
    if((udpCounterGCSsend < allowableDifference) || (udpCounterFGrecv==0) ) //FG udp queue is not delayed
    {
        udpCounterGCSsend++;

        // Send update to FlightGear
        QString cmd;
        cmd = QString("%1,%2,%3,%4,%5\n")
                .arg(ailerons) //ailerons
                .arg(elevator) //elevator
                .arg(rudder) //rudder
                .arg(throttle) //throttle
                .arg(udpCounterGCSsend); //UDP packet counter delay

        QByteArray data = cmd.toLatin1();

        if(outSocket->writeDatagram(data, QHostAddress(settings.remoteAddress), settings.outPort) == -1)
        {
            emit processOutput("Error sending UDP packet to FG: " + outSocket->errorString() + "\n");
        }
    }
    else
    {
        // don't send new packet. Flightgear cannot process UDP fast enough.
        // V1.9.1 reads udp packets at set frequency and will get delayed if packets are sent too fast
        // V2.0 does not currently work with --generic-protocol
    }
    
    if(settings.manualControlEnabled)
    {
        actData.Roll = ailerons;
        actData.Pitch = -elevator;
        actData.Yaw = rudder;
        actData.Thrust = throttle;
        actDesired->setData(actData);
    }
}


void FGSimulator::processUpdate(const QByteArray& inp)
{
    //TODO: this does not use the FLIGHT_PARAM structure, it should!
    // Split
    QString data(inp);
    QStringList fields = data.split(",");
    // Get xAccel (m/s^2)
    float xAccel = fields[FG_X_ACCEL].toFloat() * FEET2MILES;
    // Get yAccel (m/s^2)
    float yAccel = fields[FG_Y_ACCEL].toFloat() * FEET2MILES;
    // Get xAccel (m/s^2)
    float zAccel = fields[FG_Z_ACCEL].toFloat() * FEET2MILES;
    // Get pitch (deg)
    float pitch = fields[FG_PITCH].toFloat();
    // Get pitchRate (deg/s)
    float pitchRate = fields[FG_PITCH_RATE].toFloat();
    // Get roll (deg)
    float roll = fields[FG_ROLL].toFloat();
    // Get rollRate (deg/s)
    float rollRate = fields[FG_ROLL_RATE].toFloat();
    // Get yaw (deg)
    float yaw = fields[FG_YAW].toFloat();
    // Get yawRate (deg/s)
    float yawRate = fields[FG_YAW_RATE].toFloat();
    // Get latitude (deg)
    float latitude = fields[FG_LATITUDE].toFloat();
    // Get longitude (deg)
    float longitude = fields[FG_LONGITUDE].toFloat();
    // Get altitude (m)
    float altitude_msl = fields[FG_ALTITUDE_MSL].toFloat() * FEET2MILES;
    // Get altitudeAGL (m)
    float altitude_agl = fields[FG_ALTITUDE_AGL].toFloat() * FEET2MILES;
    // Get groundspeed (m/s)
    float groundspeed = fields[FG_GROUNDSPEED].toFloat() * KNOTS2M_PER_SECOND;
    // Get airspeed (m/s)
    float airspeed = fields[FG_AIRSPEED].toFloat() * KNOTS2M_PER_SECOND;
    // Get temperature (degC)
    float temperature = fields[FG_TEMPERATURE].toFloat();
    // Get pressure (kpa)
    float pressure = fields[FG_PRESSURE].toFloat() * INCHES_MERCURY2KPA;
    // Get VelocityActual Down (cm/s)
    float velocityActualDown = - fields[FG_VEL_ACT_DOWN].toFloat() * FEET_PER_SECOND2M_PER_SECOND;
    // Get VelocityActual East (cm/s)
    float velocityActualEast = fields[FG_VEL_ACT_EAST].toFloat() * FEET_PER_SECOND2M_PER_SECOND;
    // Get VelocityActual Down (cm/s)
    float velocityActualNorth = fields[FG_VEL_ACT_NORTH].toFloat() * FEET_PER_SECOND2M_PER_SECOND;

    // Get UDP packets received by FG
    int n = fields[FG_COUNTER_RECV].toInt();
    udpCounterFGrecv = n;

    ///////
    // Output formatting
    ///////
    Output2Hardware out;
    memset(&out, 0, sizeof(Output2Hardware));

    double NED[3];

    //TODO: This is broken, as currently it always yields NED = [0,0,0]
    double LLA[3] = {latitude, longitude, altitude_msl};
    double ECEF[3];
    double RNE[9];
    Utils::CoordinateConversions().LLA2Rne(LLA,(double (*)[3])RNE);
    Utils::CoordinateConversions().LLA2ECEF(LLA,ECEF);
    Utils::CoordinateConversions().LLA2NED_HomeECEF(LLA, ECEF, (double (*)[3]) RNE, NED);


    // Update GPS Position objects
    out.latitude = latitude * 1e7;
    out.longitude = longitude * 1e7;
    out.altitude = altitude_msl;
    out.agl = altitude_agl;
    out.groundspeed = groundspeed;

    out.calibratedAirspeed = airspeed;


    // Update BaroAltitude object
    out.temperature = temperature;
    out.pressure = pressure;

    // Update attActual object
    out.roll = roll;       //roll;
    out.pitch = pitch;     // pitch
    out.heading = yaw; // yaw

    out.dstN= NED[0];
    out.dstE= NED[1];
    out.dstD= NED[2];

    // Update VelocityActual.{North,East,Down}
    out.velNorth = velocityActualNorth;
    out.velEast = velocityActualEast;
    out.velDown = velocityActualDown;

    //Update gyroscope sensor data
    out.rollRate = rollRate;
    out.pitchRate = pitchRate;
    out.yawRate = yawRate;

    //Update accelerometer sensor data
    out.accX = xAccel;
    out.accY = yAccel;
    out.accZ = -zAccel;

    updateUAVOs(out);
}

