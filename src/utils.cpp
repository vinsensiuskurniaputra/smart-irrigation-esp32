#include "utils.h"

int getThresholdByPlantType(String type) {
    if (type == "cactus") return 20;
    if (type == "chili") return 40;
    if (type == "monstera") return 50;
    if (type == "spinach") return 60;
    if (type == "tomato") return 55;
    return 40;
}
