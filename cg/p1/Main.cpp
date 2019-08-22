#include "P1.h"

int
main(int argc, char** argv)
{
  return cg::Application{new P1{1280, 720}}.run(argc, argv);
}
