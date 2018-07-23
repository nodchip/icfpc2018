#pragma once

struct Vec3;
class Trace;

enum EReduction { ENoReduction, ENodchipReduction, EFofReduction };

void naive_move(const Vec3& destination, Vec3& position, Trace& trace, EReduction reduction = ENoReduction);
