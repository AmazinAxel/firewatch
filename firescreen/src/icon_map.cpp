// WMO weather code -> icon mapping (Open-Meteo uses WMO 4677 codes).
// Kept free of network/Arduino deps so the host-side preview can link it.
#include "weather.h"

namespace weather {

WeatherIcon iconFor(int code, bool isDay) {
  switch (code) {
    case 0:
      return isDay ? ICO_SUN : ICO_MOON;
    case 1:
    case 2:
      return isDay ? ICO_PARTLY_DAY : ICO_PARTLY_NIGHT;
    case 3:
      return ICO_CLOUD;
    case 45:
    case 48:
      return ICO_FOG;
    case 51:
    case 53:
    case 55:
    case 56:
    case 57:
      return ICO_DRIZZLE;
    case 61:
    case 63:
    case 65:
    case 66:
    case 67:
    case 80:
    case 81:
    case 82:
      return ICO_RAIN;
    case 71:
    case 73:
    case 75:
    case 77:
    case 85:
    case 86:
      return ICO_SNOW;
    case 95:
    case 96:
    case 99:
      return ICO_THUNDER;
    default:
      return ICO_CLOUD;
  }
}

}  // namespace weather
