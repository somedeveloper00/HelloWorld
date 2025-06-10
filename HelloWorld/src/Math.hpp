#pragma once

static float lerp(float a, float b, float t)
{
    return b * t - a * (t - 1);
}