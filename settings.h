#pragma once

#include <algorithm>
#include <yaml.h>
#include <string>
#include <optional>
#include <nlohmann/json.hpp>
#include <boost/scope_exit.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

#include <playerdb.h>


struct Settings {
  int blockPrice = 1;
  int limit      = 3;

  std::string database   = "landmanager.db";

  template <typename IO> static inline bool io(IO f, Settings &settings, YAML::Node &node) {
    return f(settings.blockPrice, node["blockPrice"]) && f(settings.limit, node["limit"]);
  }
};

inline Settings settings;

struct Vector3 {
  bool init = false;
  float X, Y, Z;

  inline Vector3(void) { init = false; }
  inline Vector3(const float x, const float y, const float z) {
    init = true;
    X    = x;
    Y    = y;
    Z    = z;
  }

  inline Vector3 operator+(const Vector3 &A) const { return Vector3(X + A.X, Y + A.Y, Z + A.Z); }

  inline Vector3 operator+(const float A) const { return Vector3(X + A, Y + A, Z + A); }

  inline float Dot(const Vector3 &A) const { return A.X * X + A.Y * Y + A.Z * Z; }

  inline int Volume(const Vector3 &A) const {
    int x2 = abs(A.X - X) + 1;
    int y2 = abs(A.Y - Y) + 1;
    int z2 = abs(A.Z - Z) + 1;

    return x2 * y2 * z2;
  }
};

extern std::unique_ptr<SQLite::Database> landDB;

namespace LandManager {
class Transaction {
  SQLite::Transaction trans;

public:
  Transaction();
  void Commit();
};

void InitDatabase();

std::optional<Mod::PlayerEntry> GetPlayerInstance(Player *player);
bool ReachedLimit(Mod::PlayerEntry owner);
bool ExistsLand(Mod::PlayerEntry owner, Vector3 start, Vector3 end);
std::optional<std::string> BuyLand(Mod::PlayerEntry owner, Vector3 start, Vector3 end);
} // namespace LandManager

enum class Action { None, Create, Buy, Exit };

extern Action action;
extern Vector3 pointA;
extern Vector3 pointB;

class CommandRegistry;
void initCommand(CommandRegistry *registry);
