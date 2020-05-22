#include "settings.h"
using json = nlohmann::json;

void LandManager::InitDatabase() {
  landDB = std::make_unique<SQLite::Database>(settings.database, SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE);
  landDB->exec(
      "CREATE TABLE IF NOT EXISTS lands "
      "(owner INTEGER, trusted TEXT, x1 REAL, y1 REAL, z1 REAL, x2 REAL, y2 REAL, z2 REAL, chkx1 REAL, chky1 REAL, chkz1 REAL, chkx2 REAL, chky2 REAL, chkz2 REAL, created_at "
      "DATETIME DEFAULT(STRFTIME('%Y-%m-%d %H:%M:%f', 'now')))");
}

std::string VecToString(Vector3 vec) {
  json obj = json::array({vec.X, vec.Y, vec.Z});
  return obj.dump();
}

Vector3 GetChunk(Vector3 vec) {
  int x = vec.X > 0 ? std::floor(vec.X / 16) : std::ceil(vec.X / 16);
  int y = vec.Y > 0 ? std::floor(vec.Y / 16) : std::ceil(vec.Y / 16);
  int z = vec.Z > 0 ? std::floor(vec.Z / 16) : std::ceil(vec.Z / 16);

  return Vector3(x, y, z);
}

std::optional<Mod::PlayerEntry> LandManager::GetPlayerInstance(Player *player) {
  auto &pdb     = Mod::PlayerDatabase::GetInstance();
  auto instance = pdb.Find(player);

  if (instance) { return instance; }

  return {};
}

bool LandManager::ReachedLimit(Mod::PlayerEntry owner) {
  static SQLite::Statement stmt{*landDB, "SELECT owner FROM lands WHERE owner = ?"};

  BOOST_SCOPE_EXIT_ALL() {
    stmt.clearBindings();
    stmt.reset();
  };

  stmt.bind(1, (int64_t) owner.xuid);

  int count = stmt.executeStep() ? stmt.getColumn(0).getInt() : 0;

  return count >= settings.limit;
}

bool LandManager::ExistsLand(Mod::PlayerEntry owner, Vector3 start, Vector3 end) {}

std::optional<std::string> LandManager::BuyLand(Mod::PlayerEntry owner, Vector3 start, Vector3 end) {
  // We store the owner, start, end and their corresponding chunks

  static SQLite::Statement stmt{
      *landDB,
      "INSERT INTO lands (owner, trusted, x1, y1, z1, x2, y2, z2, chkx1, chky1, chkz1, chkx2, chky2, chkz2) VALUES (?, "
      "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"};

  BOOST_SCOPE_EXIT_ALL() {
    stmt.clearBindings();
    stmt.tryReset();
  };

  std::string trusted = json::array({}).dump();
  Vector3 chunk1      = GetChunk(start);
  Vector3 chunk2      = GetChunk(end);

  stmt.bind(1, (int64_t) owner.xuid);
  stmt.bindNoCopy(2, trusted);
  stmt.bind(3, start.X);
  stmt.bind(4, start.Y);
  stmt.bind(5, start.Z);
  stmt.bind(6, end.X);
  stmt.bind(7, end.Y);
  stmt.bind(8, end.Z);
  stmt.bind(9, chunk1.X);
  stmt.bind(10, chunk1.Y);
  stmt.bind(11, chunk1.Z);
  stmt.bind(12, chunk2.X);
  stmt.bind(13, chunk2.Y);
  stmt.bind(14, chunk2.Z);

  try {
    stmt.exec();
    return {};
  } catch (SQLite::Exception const &ex) { return ex.what(); }
}

LandManager::Transaction::Transaction() : trans(*landDB) {}
void LandManager::Transaction::Commit() { trans.commit(); }
