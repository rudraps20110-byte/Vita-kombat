#pragma once

#include "types.hpp"

namespace vk {

const CharacterDef& getCharacter(int id);
const StageDef& getStage(int id);
MoveData getMoveData(int charId, MoveKind kind);

} // namespace vk

