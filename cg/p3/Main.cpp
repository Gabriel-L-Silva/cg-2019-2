#include "P3.h"

int
main(int argc, char** argv)
{
  return cg::Application{new P3{1280, 720}}.run(argc, argv);
}
