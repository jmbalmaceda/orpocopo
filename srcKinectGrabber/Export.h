#include <iostream>
#include <string>
#include <string.h>
#include <fstream>

using namespace std;


string IntToString(int value);

class Export
{
public:
  static void ExportToCSV(string fileName, string data, bool append);

  
};