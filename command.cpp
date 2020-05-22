#include <economy.h>
#include <command.h>
#include <Command/Command.h>
#include <Command/CommandOrigin.h>
#include <Command/CommandOutput.h>
#include <Command/CommandRegistry.h>

#include "settings.h"

void reset() {
  pointA = Vector3();
  pointB = Vector3();
  action = Action::None;
}

bool isInside(Vector3 point, Vector3 start, Vector3 end) {
  float xMin = std::min(start.X, end.X);
  float yMin = std::min(start.X, end.X);
  float zMin = std::min(start.Z, end.Z);
  float xMax = std::max(start.X, end.X);
  float yMax = std::max(start.X, end.X);
  float zMax = std::max(start.Z, end.Z);

  if (point.X < xMin || point.X > xMax || point.Y < yMin || point.Y > yMax || point.Z < zMin || point.Z > zMax) {
    return false;
  }

  return true;
}

class LandManagerCommand : public Command {
public:
  Action target_action;

  LandManagerCommand() {}

  void execute(CommandOrigin const &origin, CommandOutput &output) {
    action = target_action;
    std::ostringstream re;

    if (action == Action::Create) {
      output.success("[Zonas] Selecciona el punto inicial o sal con '/land exit'");
    } else if (action == Action::Buy) {
      // If we havent selected two points yet, return
      if (!pointA.init || !pointB.init) {
        output.error("[Zonas] Debes seleccionar dos puntos primero.");
        return;
      }

      // Try to get the player instance and just reuse it
      auto player    = (Player *) origin.getEntity();
      auto pInstance = LandManager::GetPlayerInstance(player);

      if (!pInstance) {
        output.error("[Zonas] Error al realizar accion. Consulte al admin.");
        return;
      }

      // Check if we have enough money
      auto balance = Mod::Economy::GetBalance(player);
      int price    = pointA.Volume(pointB) * settings.blockPrice;

      if (price > balance) {
        output.success("[Zonas] No cuentas con suficiente dinero.");
        return;
      }

      // Check if we reached the limit of land we can own
      bool limit = LandManager::ReachedLimit(*pInstance);

      if (limit) {
        output.error("[Zonas] Haz alcanzado el limite de zonas.");
        return;
      }

      // Check if we are overlapping any other land
      bool overlap = LandManager::ExistsLand(*pInstance, pointA, pointB);

      if (overlap) {
        output.error("[Zonas] Estas encima de una zona ya existente.");
        return;
      }

      // Finally, try and buy the land
      Mod::Economy::Transaction ecoTrans;
      LandManager::Transaction landTrans;

      auto err = Mod::Economy::UpdateBalance(player, balance - price, "Land Bought.");
      if (err) {
        output.error("[Zonas] Ocurrio un error al realizar la compra, intente de nuevo.");
        return;
      }

      auto err2 = LandManager::BuyLand(*pInstance, pointA, pointB);
      if (err2) {
        output.error("[Zonas] Ocurrio un error al realizar la compra, intente de nuevo.");
        return;
      }

      ecoTrans.Commit();
      landTrans.Commit();

      output.success("[Zonas] Has comprado la zona.");

      reset();
    } else if (action == Action::Exit) {
      reset();
    }
  }

  static void setup(CommandRegistry *registry) {
    using namespace commands;

    registry->registerCommand(
        "land", "Administracion de zonas", CommandPermissionLevel::Any, CommandFlagCheat, CommandFlagNone);

    addEnum<Action>(
        registry, "land-option", {{"create", Action::Create}, {"buy", Action::Buy}, {"exit", Action::Exit}});

    registry->registerOverload<LandManagerCommand>(
        "land", mandatory<CommandParameterDataType::ENUM>(&LandManagerCommand::target_action, "action", "land-option"));
  }
};

void initCommand(CommandRegistry *registry) { LandManagerCommand::setup(registry); }