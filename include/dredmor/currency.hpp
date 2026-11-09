#pragma once
#include "dredmor/abilities.hpp"
#include "dredmor/selection.hpp"
#include "dredmor/inventory.hpp"

namespace dredmor {
struct CurrencyObject {
    ItemKind kind=ItemKind::currency;
    std::int32_t value=0,quantity=1;
    bool stackable=false;
    std::string sprite_key,sprite;
};
CurrencyObject make_currency(std::int32_t amount,const std::vector<SkillRank>& skills={},
                             const std::vector<AbilityDefinition>& abilities={});
InventoryItem make_currency_item(const ItemDefinition& definition,std::uint32_t index,std::uint64_t instance,
                                  std::int32_t amount,const std::vector<SkillRank>& skills={},
                                  const std::vector<AbilityDefinition>& abilities={});
// Shared item-factory override changes Item quantity, not the separate money value.
void currency_quantity_override(CurrencyObject& object,int amount);
std::int32_t roll_currency(SpawnDepthContext depth,DarwinRandom& random);
std::int32_t roll_named_currency(int depth,int gold_per_level,DarwinRandom& random);
enum class CurrencyOperation {give,take};
struct CurrencyTransfer {std::int32_t balance=0,delta=0;bool money_sound=false;};
// Original transfers allow a negative balance and use wrapping 32-bit arithmetic.
// INT_MIN would recurse indefinitely through the original virtual give/take pair;
// the port rejects that input before mutation.
CurrencyTransfer transfer_currency(std::int32_t balance,std::int32_t amount,CurrencyOperation operation);
}
