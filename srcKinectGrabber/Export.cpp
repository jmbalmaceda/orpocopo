#include "Export.h"
#include <iostream>
#include <string>
#include <string.h>
#include <fstream>
using namespace std;


string IntToString(int value)
{
	    char convC[10];
		itoa(value,convC,10);
		std::string convert(convC);
		return convert;
	}



void Export::ExportToCSV(string fileName, string data,bool append)
{
  ofstream myfile;
  if (append)
	 myfile.open(fileName, ios::out | ios::app);
  else
	 myfile.open(fileName);

  if (myfile.is_open())
  {	 	
	  myfile << data << "\n";
	  myfile.close();
  }
  
}