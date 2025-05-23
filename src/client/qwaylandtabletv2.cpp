// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandtabletv2_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylanddisplay_p.h"
#include "qwaylandsurface_p.h"
#include <QtGui/private/qpointingdevice_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcQpaInputDevices, "qt.qpa.input.devices")
Q_DECLARE_LOGGING_CATEGORY(lcQpaWaylandInput)

QWaylandTabletManagerV2::QWaylandTabletManagerV2(QWaylandDisplay *display, uint id, uint version)
    : zwp_tablet_manager_v2(display->wl_registry(), id, qMin(version, uint(1)))
{
    qCDebug(lcQpaInputDevices, "new tablet manager: ID %d version %d", id, version);
}

QWaylandTabletManagerV2::~QWaylandTabletManagerV2()
{
    destroy();
}

QWaylandTabletSeatV2::QWaylandTabletSeatV2(QWaylandTabletManagerV2 *manager, QWaylandInputDevice *seat)
    : QtWayland::zwp_tablet_seat_v2(manager->get_tablet_seat(seat->wl_seat()))
    , m_seat(seat)
{
    qCDebug(lcQpaInputDevices) << "new tablet seat" << seat->seatname() << "id" << seat->id();
}

QWaylandTabletSeatV2::~QWaylandTabletSeatV2()
{
    for (auto *tablet : m_tablets)
        tablet->destroy();
    for (auto *tool : m_tools)
        tool->destroy();
    for (auto *pad : m_pads)
        pad->destroy();
    qDeleteAll(m_tablets);
    qDeleteAll(m_tools);
    qDeleteAll(m_deadTools);
    qDeleteAll(m_pads);
    destroy();
}

void QWaylandTabletSeatV2::zwp_tablet_seat_v2_tablet_added(zwp_tablet_v2 *id)
{
    auto *tablet = new QWaylandTabletV2(id, m_seat->seatname());
    qCDebug(lcQpaInputDevices) << "seat" << this << id << "has tablet" << tablet;
    tablet->setParent(this);
    m_tablets.push_back(tablet);
    connect(tablet, &QWaylandTabletV2::destroyed, this, [this, tablet] { m_tablets.removeOne(tablet); });
}

void QWaylandTabletSeatV2::zwp_tablet_seat_v2_tool_added(zwp_tablet_tool_v2 *id)
{
    qDeleteAll(m_deadTools);
    auto *tool = new QWaylandTabletToolV2(this, id);
    if (m_tablets.size() == 1) {
        tool->setParent(m_tablets.first());
        QPointingDevicePrivate *d = QPointingDevicePrivate::get(tool);
        d->name = m_tablets.first()->name() + u" stylus";
    } else {
        qCDebug(lcQpaInputDevices) << "seat" << this << "has tool" << tool << "for one of these tablets:" << m_tablets;
        // TODO identify which tablet if there are several; then tool->setParent(tablet)
    }
    m_tools.push_back(tool);
    connect(tool, &QWaylandTabletToolV2::destroyed, this, [this, tool] {
        m_tools.removeOne(tool);
        m_deadTools.removeOne(tool);
    });
}

void QWaylandTabletSeatV2::zwp_tablet_seat_v2_pad_added(zwp_tablet_pad_v2 *id)
{
    auto *pad = new QWaylandTabletPadV2(id);
    if (m_tablets.size() == 1) {
        pad->setParent(m_tablets.first());
        QPointingDevicePrivate *d = QPointingDevicePrivate::get(pad);
        d->name = m_tablets.first()->name() + u" touchpad";
    } else {
        qCDebug(lcQpaInputDevices) << "seat" << this << "has touchpad" << pad << "for one of these tablets:" << m_tablets;
        // TODO identify which tablet if there are several
    }
    m_pads.push_back(pad);
    connect(pad, &QWaylandTabletPadV2::destroyed, this, [this, pad] { m_pads.removeOne(pad); });
}

QWaylandTabletV2::QWaylandTabletV2(::zwp_tablet_v2 *tablet, const QString &seatName)
    : QPointingDevice(u"unknown"_s, -1, DeviceType::Stylus, PointerType::Pen,
                      Capability::Position | Capability::Hover,
                      1, 1)
    , QtWayland::zwp_tablet_v2(tablet)
{
    qCDebug(lcQpaInputDevices) << "new tablet on seat" << seatName;
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    d->seatName = seatName;
}

void QWaylandTabletV2::zwp_tablet_v2_name(const QString &name)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    d->name = name;
}

void QWaylandTabletV2::zwp_tablet_v2_id(uint32_t vid, uint32_t pid)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    d->systemId = (quint64(vid) << 32) | pid;
    qCDebug(lcQpaInputDevices) << "vid" << vid << "pid" << pid << "stored as systemId in" << this;
}

void QWaylandTabletV2::zwp_tablet_v2_path(const QString &path)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    d->busId = path;
}

void QWaylandTabletV2::zwp_tablet_v2_done()
{
    QWindowSystemInterface::registerInputDevice(this);
}

void QWaylandTabletSeatV2::toolRemoved(QWaylandTabletToolV2 *tool)
{
    m_tools.removeOne(tool);
    m_deadTools.append(tool);
}

void QWaylandTabletV2::zwp_tablet_v2_removed()
{
    destroy();
    deleteLater();
}

QWaylandTabletToolV2::QWaylandTabletToolV2(QWaylandTabletSeatV2 *tabletSeat, ::zwp_tablet_tool_v2 *tool)
    : QPointingDevice(u"tool"_s, -1, DeviceType::Stylus, PointerType::Pen,
                      Capability::Position | Capability::Hover,
                      1, 1)
    , QtWayland::zwp_tablet_tool_v2(tool)
    , m_tabletSeat(tabletSeat)
{
    // TODO get the number of buttons somehow?
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_type(uint32_t tool_type)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);

    switch (tool_type) {
    case type_airbrush:
    case type_brush:
    case type_pencil:
    case type_pen:
        d->pointerType = QPointingDevice::PointerType::Pen;
        break;
    case type_eraser:
        d->pointerType = QPointingDevice::PointerType::Eraser;
        break;
    case type_mouse:
    case type_lens:
        d->pointerType = QPointingDevice::PointerType::Cursor;
        break;
    case type_finger:
        d->pointerType = QPointingDevice::PointerType::Unknown;
        break;
    }

    switch (tool_type) {
    case type::type_airbrush:
        d->deviceType = QInputDevice::DeviceType::Airbrush;
        d->capabilities.setFlag(QInputDevice::Capability::TangentialPressure);
        break;
    case type::type_brush:
    case type::type_pencil:
    case type::type_pen:
    case type::type_eraser:
        d->deviceType = QInputDevice::DeviceType::Stylus;
        break;
    case type::type_lens:
        d->deviceType = QInputDevice::DeviceType::Puck;
        break;
    case type::type_mouse:
    case type::type_finger:
        d->deviceType = QInputDevice::DeviceType::Unknown;
        break;
    }
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_hardware_serial(uint32_t hardware_serial_hi, uint32_t hardware_serial_lo)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    d->uniqueId = QPointingDeviceUniqueId::fromNumericId((quint64(hardware_serial_hi) << 32) + hardware_serial_lo);
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_hardware_id_wacom(uint32_t hardware_id_hi, uint32_t hardware_id_lo)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    d->systemId = (quint64(hardware_id_hi) << 32) + hardware_id_lo;
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_capability(uint32_t capability)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    switch (capability) {
    case capability_tilt:
        // no distinction... we have to assume it has both axes
        d->capabilities.setFlag(QInputDevice::Capability::XTilt);
        d->capabilities.setFlag(QInputDevice::Capability::YTilt);
        break;
    case capability_pressure:
        d->capabilities.setFlag(QInputDevice::Capability::Pressure);
        break;
    case capability_distance:
        d->capabilities.setFlag(QInputDevice::Capability::ZPosition);
        break;
    case capability_rotation:
        d->capabilities.setFlag(QInputDevice::Capability::Rotation);
        break;
    case capability_slider:
        // nothing to represent that so far
        break;
    case capability_wheel:
        d->capabilities.setFlag(QInputDevice::Capability::Scroll);
        d->capabilities.setFlag(QInputDevice::Capability::PixelScroll);
        break;
    }
    qCDebug(lcQpaInputDevices) << capability << "->" << this;
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_done()
{
    QWindowSystemInterface::registerInputDevice(this);
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_removed()
{
    destroy();
    m_tabletSeat->toolRemoved(this);
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_proximity_in(uint32_t serial, zwp_tablet_v2 *tablet, wl_surface *surface)
{
    Q_UNUSED(tablet);

    m_tabletSeat->seat()->mSerial = serial;

    if (Q_UNLIKELY(!surface)) {
        qCDebug(lcQpaWayland) << "Ignoring zwp_tablet_tool_v2_proximity_v2 with no surface";
        return;
    }
    m_pending.enteredSurface = true;
    m_pending.proximitySurface = QWaylandSurface::fromWlSurface(surface);
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_proximity_out()
{
    m_pending.enteredSurface = false;
    m_pending.proximitySurface = nullptr;
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_down(uint32_t serial)
{
    m_pending.down = true;

    m_tabletSeat->seat()->mSerial = serial;

    if (m_pending.proximitySurface) {
        if (QWaylandWindow *window = m_pending.proximitySurface->waylandWindow()) {
            QWaylandInputDevice *seat = m_tabletSeat->seat();
            seat->display()->setLastInputDevice(seat, serial, window);
        }
    }
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_up()
{
    m_pending.down = false;
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_motion(wl_fixed_t x, wl_fixed_t y)
{
    m_pending.surfacePosition = QPointF(wl_fixed_to_double(x), wl_fixed_to_double(y));
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_pressure(uint32_t pressure)
{
    const int maxPressure = 65535;
    m_pending.pressure = qreal(pressure)/maxPressure;
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_distance(uint32_t distance)
{
    m_pending.distance = distance;
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_tilt(wl_fixed_t tilt_x, wl_fixed_t tilt_y)
{
    m_pending.xTilt = wl_fixed_to_double(tilt_x);
    m_pending.yTilt = wl_fixed_to_double(tilt_y);
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_rotation(wl_fixed_t degrees)
{
    m_pending.rotation = wl_fixed_to_double(degrees);
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_slider(int32_t position)
{
    m_pending.slider = qreal(position) / 65535;
}

static Qt::MouseButton mouseButtonFromTablet(uint button)
{
    switch (button) {
    case 0x110: return Qt::MouseButton::LeftButton; // BTN_LEFT
    case 0x14b: return Qt::MouseButton::MiddleButton; // BTN_STYLUS
    case 0x14c: return Qt::MouseButton::RightButton; // BTN_STYLUS2
    default:
        return Qt::NoButton;
    }
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_button(uint32_t serial, uint32_t button, uint32_t state)
{
    m_tabletSeat->seat()->mSerial = serial;

    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    Qt::MouseButton mouseButton = mouseButtonFromTablet(button);
    if (state == button_state_pressed)
        m_pending.buttons |= mouseButton;
    else
        m_pending.buttons &= ~mouseButton;
    // ideally we'd get button count when the tool is discovered; seems to be a shortcoming in tablet-unstable-v2
    // but if we get events from buttons we didn't know existed, increase it
    if (mouseButton == Qt::RightButton)
        d->buttonCount = qMax(d->buttonCount, 2);
    else if (mouseButton == Qt::MiddleButton)
        d->buttonCount = qMax(d->buttonCount, 3);
}

void QWaylandTabletToolV2::zwp_tablet_tool_v2_frame(uint32_t time)
{
    if (!m_pending.proximitySurface) {
        if (m_applied.enteredSurface) {
            // leaving proximity
            QWindowSystemInterface::handleTabletEnterLeaveProximityEvent(nullptr, this, false);
            m_pending = State(); // Don't leave pressure etc. lying around when we enter the next surface
            m_applied = State();
        } else {
            qCWarning(lcQpaWayland) << "Can't send tablet event with no proximity surface, ignoring";
        }
        return;
    }

    QWaylandWindow *waylandWindow = QWaylandWindow::fromWlSurface(m_pending.proximitySurface->object());
    QWindow *window = waylandWindow->window();

    if (!m_applied.proximitySurface) {
        // TODO get position etc. as below
        QWindowSystemInterface::handleTabletEnterLeaveProximityEvent(window, this, true);
        m_applied.proximitySurface = m_pending.proximitySurface;
    }

    if (!(m_pending == m_applied)) {
        ulong timestamp = time;
        const QPointF localPosition = waylandWindow->mapFromWlSurface(m_pending.surfacePosition);

        QPointF delta = localPosition - localPosition.toPoint();
        QPointF globalPosition = window->mapToGlobal(localPosition.toPoint());
        globalPosition += delta;

        Qt::MouseButtons buttons = m_pending.down ? Qt::MouseButton::LeftButton : Qt::MouseButton::NoButton;
        buttons |= m_pending.buttons;
        qreal pressure = m_pending.pressure;
        int xTilt = int(m_pending.xTilt);
        int yTilt = int(m_pending.yTilt);
        qreal tangentialPressure = m_pending.slider;
        qreal rotation = m_pending.rotation;
        int z = int(m_pending.distance);

        // do not use localPosition here since that is in Qt window coordinates
        // but we need surface coordinates to include the decoration
        bool decorationHandledEvent = waylandWindow->handleTabletEventDecoration(
                m_tabletSeat->seat(), m_pending.surfacePosition,
                window->mapToGlobal(m_pending.surfacePosition) + delta, buttons,
                m_tabletSeat->seat()->modifiers());

        if (!decorationHandledEvent) {
            QWindowSystemInterface::handleTabletEvent(window, timestamp, this, localPosition, globalPosition,
                                                      buttons, pressure,
                                                      xTilt, yTilt, tangentialPressure, rotation, z,
                                                      m_tabletSeat->seat()->modifiers());
        }
    }

    m_applied = m_pending;
}

// TODO: delete when upgrading to c++20
bool QWaylandTabletToolV2::State::operator==(const QWaylandTabletToolV2::State &o) const {
    return
            down == o.down &&
            proximitySurface.data() == o.proximitySurface.data() &&
            enteredSurface == o.enteredSurface &&
            surfacePosition == o.surfacePosition &&
            distance == o.distance &&
            pressure == o.pressure &&
            rotation == o.rotation &&
            xTilt == o.xTilt &&
            yTilt == o.yTilt &&
            slider == o.slider &&
            buttons == o.buttons;
}

QWaylandTabletPadV2::QWaylandTabletPadV2(::zwp_tablet_pad_v2 *pad)
    : QPointingDevice(u"tablet touchpad"_s, -1, DeviceType::TouchPad, PointerType::Finger,
                      Capability::Position,
                      1, 1)
    , QtWayland::zwp_tablet_pad_v2(pad)
{
}

void QWaylandTabletPadV2::zwp_tablet_pad_v2_path(const QString &path)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    d->busId = path;
}

void QWaylandTabletPadV2::zwp_tablet_pad_v2_buttons(uint32_t buttons)
{
    QPointingDevicePrivate *d = QPointingDevicePrivate::get(this);
    d->buttonCount = buttons;
}

void QWaylandTabletPadV2::zwp_tablet_pad_v2_done()
{
    QWindowSystemInterface::registerInputDevice(this);
}

void QWaylandTabletPadV2::zwp_tablet_pad_v2_removed()
{
    destroy();
    delete this;
}

} // namespace QtWaylandClient

QT_END_NAMESPACE

#include "moc_qwaylandtabletv2_p.cpp"
