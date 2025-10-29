/*
 * Copyright 2025 Kristóf Marussy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <backend/hidpp20/features/HapticFeedback.h>

using namespace logid::backend::hidpp20;

static const uint8_t kVibration = 0x01;
static const uint8_t kBatterySaving = 0x02;

HapticFeedback::HapticFeedback(Device* dev) : Feature(dev, ID) {
}

void HapticFeedback::setStrength(uint8_t strength, bool enabled, bool battery_saving) {
  uint8_t flags = 0;
  if (enabled) {
    flags |= kVibration;
  }
  if (battery_saving) {
    flags |= kBatterySaving;
  }
  std::vector<uint8_t> params = {flags, strength};
  callFunction(SetStrength, params);
}

void HapticFeedback::playEffect(uint8_t effect) {
    std::vector<uint8_t> params = {effect};
    callFunction(PlayEffect, params);
}
