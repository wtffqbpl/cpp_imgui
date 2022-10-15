#include "application.h"

int main() {
  auto *app = new Application{};
  app->run();
  delete app;

  return 0;
}