#include "P4.h"

int
main(int argc, char** argv)
{
  return cg::Application{new P4{1280, 720}}.run(argc, argv);
}
