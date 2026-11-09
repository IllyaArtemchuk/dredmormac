#include "dredmor/currency.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace dredmor {
namespace {
std::int32_t signed_bits(std::uint32_t bits) {std::int32_t value;std::memcpy(&value,&bits,4);return value;}
std::int32_t truncate(float value) {
    if (!std::isfinite(value) || value>=2147483648.0f || value< -2147483648.0f) return std::numeric_limits<std::int32_t>::min();
    return std::int32_t(value);
}
}
CurrencyObject make_currency(std::int32_t amount,const std::vector<SkillRank>& skills,const std::vector<AbilityDefinition>& abilities) {
    for (const auto& ability:abilities) {
        int rank=0;
        for (std::size_t i=0;i<std::min<std::size_t>(7,skills.size());++i) if (skills[i].id==ability.skill_id) {rank=skills[i].rank;break;}
        if (ability.level<rank && ability.currency_bonus>0) {
            const float base=float(amount),extra=ability.currency_bonus*base;
            amount=truncate(extra+base);
        }
    }
    int image=0;
    for (int threshold:{10,50,250,1000,4000}) if (amount>=threshold) ++image;
    CurrencyObject result;result.value=amount;result.sprite_key="cash"+std::to_string(image+1);
    result.sprite="items/cash"+std::to_string(image)+".png";return result;
}
void currency_quantity_override(CurrencyObject& object,int amount) {
    if (amount>1) {object.quantity=amount;object.stackable=true;}
}
std::int32_t roll_currency(SpawnDepthContext depth,DarwinRandom& random) {
    const auto level=depth.alternate && depth.alternate_depth>=0?depth.alternate_depth:depth.depth;
    if (level<0) return 1;
    return signed_bits((std::uint32_t(level)+1U)*(random.bounded(15)+2U));
}
std::int32_t roll_named_currency(int depth,int gold_per_level,DarwinRandom& random) {
    const auto count=(std::uint32_t(depth)+1U)*std::uint32_t(gold_per_level);
    return signed_bits(random.bounded(count)+5U);
}
CurrencyTransfer transfer_currency(std::int32_t balance,std::int32_t amount,CurrencyOperation operation) {
    if (amount==std::numeric_limits<std::int32_t>::min()) throw std::overflow_error("currency transfer would recurse indefinitely");
    const auto delta=operation==CurrencyOperation::give?amount:-amount;
    return {signed_bits(std::uint32_t(balance)+std::uint32_t(delta)),delta,
        operation==CurrencyOperation::give?amount>=0:amount<0};
}
}
