// ConsoleApplication3.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Common.h"


int main(int argc, char *argv[])
{
   if (argc <= 1) {
      std::cout << "-c or -s" << std::endl;
      return 0;
   }
   bool isServer = (0 == strcmp(argv[1], "-s"));
   bool isClient = (0 == strcmp(argv[1], "-c"));
   if ((isServer ^ isClient) == 0) {
      std::cout << "-c or (exculsive or) -s" << std::endl;
   }

   if (isServer) {
      return ServerMain();
   } else {
      return ClientMain();
   }
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
