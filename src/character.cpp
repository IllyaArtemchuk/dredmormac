#include "dredmor/character.hpp"
#include "dredmor/selection.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace dredmor {
class CharacterBuilder {
public:
    CharacterBuilder(const GameContent& content,World world,const std::vector<int>& selected,std::uint32_t seed,Difficulty difficulty)
        :content_(content),ranks_(content.starting_ranks(selected)),
         session_(std::move(world),make_player(ranks_),seed,content.tweaks().integer(difficulty,"experience n")) {
        std::vector<SkillRank> skills;
        // Store selected skills in the same database order as the original.
        for (const auto& skill:content.skills()) if (std::find(selected.begin(),selected.end(),skill.id)!=selected.end())
            skills.push_back({skill.id,1});
        session_.character_profile(ranks_,std::make_shared<const std::vector<ItemDefinition>>(content.items()),
            std::move(skills),std::make_shared<const std::vector<AbilityDefinition>>(content.abilities()));
        for (const auto& grant:content.starting_loadout(selected)) give(grant.item);
        if (ranks_[0]>=ranks_[1] && ranks_[0]>=ranks_[2]) give_armour("Leather Cuirass");
        else if (ranks_[1]>=ranks_[0] && ranks_[1]>=ranks_[2]) {if (!head_given_) give_armour("Robe");}
        else give_armour("Fine Linen Shirt");
        std::vector<std::size_t> food_indices;std::vector<FoodCandidate> food;
        for (std::size_t i=0;i<content.items().size();++i) {
            const auto& definition=content.items()[i];
            if (definition.kind==ItemKind::food) {
                food_indices.push_back(i);food.push_back({definition.food_mp,definition.level,definition.special,definition.debug});
            }
        }
        for (int i=0;i<2;++i) {
            put(make(food_indices.at(select_booze(food,session_.random_.libc))));
            put(make(food_indices.at(select_edible(food,session_.random_.libc))));
        }
        session_.rebuild_equipment();session_.player_.health=session_.player_.secondary[0];session_.player_.mana=session_.player_.secondary[1];
    }
    Session finish() { return std::move(session_); }
private:
    const GameContent& content_;
    ClassRanks ranks_;
    Session session_;
    std::uint64_t next_instance_=1;
    bool weapon_given_=false,head_given_=false;
    std::optional<std::size_t> find(std::string_view name) const {
        std::optional<std::size_t> found;
        for (std::size_t i=0;i<content_.items().size();++i) if (content_.items()[i].name==name) {
            if (found) throw std::runtime_error("ambiguous starting item: "+std::string(name));found=i;
        }
        return found;
    }
    InventoryItem make(std::size_t index,int amount=1) {
        if (next_instance_==std::numeric_limits<std::uint64_t>::max()) throw std::runtime_error("starting item IDs exhausted");
        auto item=make_inventory_item(content_.items().at(index),static_cast<std::uint32_t>(index),next_instance_++,session_.random_.libc);
        if (amount>1) item.quantity=amount;
        return item;
    }
    void put(const InventoryItem& item) {
        if (session_.inventory_.insert(item).outcome==InsertOutcome::full) throw std::runtime_error("starting inventory is full");
    }
    void wear(const InventoryItem& item,EquipmentSlot slot) {
        auto displaced=session_.equipment_.equip(item,slot,*session_.definitions_);
        if (displaced) throw std::logic_error("starting equipment unexpectedly replaced an item");
    }
    void give_armour(std::string_view name) {
        const auto index=find(name);if (!index) return; // Original helper ignores missing names.
        const auto item=make(*index);const auto& definition=content_.items()[*index];
        bool equipped=false;
        for (int number:{2,5,0,7,6,3,10,11,9}) {
            const auto slot=static_cast<EquipmentSlot>(number);
            if (!definition.can_equip(slot)) continue;
            if (!session_.equipment_.slots()[number]) {wear(item,slot);equipped=true;}
            break;
        }
        if (!equipped) put(item);
        if (session_.equipment_.slots()[0]) head_given_=true;
    }
    void give_weapon(const SkillLoadout& grant) {
        const auto index=find(grant.subtype);
        const bool automatic=!weapon_given_;weapon_given_=true;
        if (!index) {
            if (grant.amount>1) throw std::runtime_error("stacked starting weapon is missing");
            return;
        }
        // Original creates the requested multi-item stack, then separately
        // gives one weapon. For ammunition the extra unit can merge into it.
        if (grant.amount>1) put(make(*index,grant.amount));
        auto item=make(*index);const auto& definition=content_.items()[*index];
        std::optional<EquipmentSlot> target;
        if (automatic) {
            if (definition.weapon_type==4 && !session_.equipment_.slots()[8]) target=EquipmentSlot::ranged;
            else if (definition.weapon_type!=5 && !session_.equipment_.slots()[1]) target=EquipmentSlot::hand_left;
        }
        if (target) {
            if (!definition.can_equip(*target)) throw std::runtime_error("unsupported automatic weapon anatomy path");
            wear(item,*target);
        } else put(item);
    }
    void give(const SkillLoadout& grant) {
        if (grant.type=="weapon") {give_weapon(grant);return;}
        if (grant.type=="armour") {give_armour(grant.subtype);return;}
        if (grant.type=="misc") {
            const int subtype=grant.subtype=="lockpick" || grant.subtype=="lockpicks"?0:grant.subtype=="spores"?1:-1;
            if (subtype<0) return;
            for (std::size_t i=0;i<content_.items().size();++i) if (content_.items()[i].builtin_subtype==subtype) {put(make(i));return;}
            throw std::runtime_error("starting builtin item is unavailable");
        }
        if (!grant.subtype.empty() && grant.subtype!="random") {
            const auto index=find(grant.subtype);
            if (!index) throw std::runtime_error("named starting item is unavailable: "+grant.subtype);
            put(make(*index,grant.amount));return;
        }
        std::vector<std::size_t> pool;
        for (std::size_t i=0;i<content_.items().size();++i) {
            const auto& item=content_.items()[i];
            if (((grant.type=="reagent" || grant.type=="component") && item.alchemical) ||
                (grant.type=="mushroom" && item.kind==ItemKind::mushroom) ||
                (grant.type=="potion" && item.kind==ItemKind::potion) || (grant.type=="wand" && item.kind==ItemKind::wand)) pool.push_back(i);
        }
        if (pool.empty()) throw std::runtime_error("empty or unsupported starting item pool: "+grant.type);
        const auto choice=session_.random_.libc.bounded(static_cast<std::uint32_t>(pool.size()));
        put(make(pool[choice],grant.amount));
    }
};
Session create_character(const GameContent& content,World world,const std::vector<int>& selected,std::uint32_t seed,Difficulty difficulty) {
    return CharacterBuilder(content,std::move(world),selected,seed,difficulty).finish();
}
}
