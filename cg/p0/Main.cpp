#include "P0.h"

int
main(int argc, char** argv)
{
  return cg::Application{new P0{1280, 720}}.run(argc, argv);
}
