#pragma once
#include <cstdint>
#include <istream>
#include <string>
#include <vector>

namespace dredmor {
// Provisional harness policy, pending comparison with the original executable.
struct Position { int x; int y; };
enum class Action { north, south, west, east, wait };
enum class Outcome { moved, waited, blocked };
class World {
public:
    static World read(std::istream& input);
    static World restore(std::istream& input,std::uint64_t turn);
    Outcome step(Action action);
    bool passable(Position p) const;
    Position player() const { return player_; }
    std::uint64_t turn() const { return turn_; }
    int width() const { return static_cast<int>(rows_[0].size()); }
    int height() const { return static_cast<int>(rows_.size()); }
    char tile(int x, int y) const { return rows_.at(y).at(x); }
    std::string snapshot() const;
    std::string room_text() const;
private:
    std::vector<std::string> rows_;
    Position player_{0, 0};
    std::uint64_t turn_ = 0;
};
Action parse_action(char command);
}
