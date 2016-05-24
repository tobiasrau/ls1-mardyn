#include "grideval.h"
#include "Domain.h"

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <cstdlib>
#include <cmath>
#include <cstring>

int main(int argc, char** argv)
{
   const char* usage = " <prefix> -g <grid size> -l <box size> [-d <digits>] [-i] [-j <frames>] [-m <max-threshold>] [-r <first> <last>] [-s <suffix>]\n\n-d\tdigits used for frame numbering\n-i\tintegral (i.e. single) cavity file\n-m\tmaximal considered threshold size\n-r\tselected range (initial and terminal frame numbers)\n";
   if((argc < 6) || (argc > 19))
   {
      cout << "There should be at least five and at most 18 arguments.\n\n";
      cout << argv[0] << usage;
      exit(7);
   }
   
   char* prefix = argv[1];
   
   bool grid_specified = false;
   int grid;
   bool length_specified = false;
   double length;
   
   bool integral = false;
   unsigned jump = 1;
   unsigned first_frame = 1;
   bool last_specified = false;
   unsigned last_frame = 0;
   bool suffix_specified = false;
   char* suffix = (char*)0;
   unsigned digits_specified = false;
   unsigned digits = 0;
   
   unsigned num_threshold = 11;
   unsigned min_threshold = 3;
   unsigned max_threshold = 3000;
   
   for(int i=2; i < argc; i++)
   {
      if(*argv[i] != '-')
      {
         cout << "\nFlag expected where '" << argv[i]
              << "' was given.\n\n";
         cout << argv[0] << usage;
         exit(8);
      }
      for(int j=1; argv[i][j]; j++)
      {
         if(argv[i][j] == 'd')
         {
            i++;
            digits = atoi(argv[i]);
            if((digits < 1) || (digits > 12))
            {
               cout << "\nToo many digits.\n\n";
               cout << argv[0] << usage;
               exit(11);
            }
            digits_specified = true;
            break;
         }
         else if(argv[i][j] == 'g')
         {
            i++;
            grid = atoi(argv[i]);
            grid_specified = true;
            break;
         }
         else if(argv[i][j] == 'i')
         {
            integral = true;
         }
         else if(argv[i][j] == 'j')
         {
            i++;
            jump = atoi(argv[i]);
            break;
         }
         else if(argv[i][j] == 'l')
         {
            i++;
            length = atof(argv[i]);
            length_specified = true;
            break;
         }
         else if(argv[i][j] == 'm')
         {
            i++;
            max_threshold = atoi(argv[i]);
            if(max_threshold < 23)
            {
               cout << "\nMaximal threshold too small.\n\n";
               cout << argv[0] << usage;
               exit(10);
            }
            break;
         }
         else if(argv[i][j] == 'r')
         {
            i++;
            first_frame = atoi(argv[i]);
            i++;
            last_frame = atoi(argv[i]);
            last_specified = true;
            break;
         }
         else if(argv[i][j] == 's')
         {
            i++;
            suffix = argv[i];
            suffix_specified = true;
            break;
         }
         else
         {
            cout << "\nUnknown flag \"-" << argv[i][j] << ".\n\n"
                 << argv[0] << usage;
            exit(9);
         }
      }
   }
   
   if(!grid_specified)
   {
      cout << "\nUnknown grid size.\n\n" << argv[0] << usage;
      exit(13);
   }
   if(!length_specified)
   {
      cout << "\nUnknown box size.\n\n" << argv[0] << usage;
      exit(14);
   }
   
   cout << "# frame\t\tN_cav\t";
   map<unsigned, unsigned> thresholds;
   double thrfactor = pow((double)max_threshold/min_threshold, 1.0/(num_threshold - 1.0));
   double tthreshold = min_threshold;
   for(unsigned i = 0; i < num_threshold; i++)
   {
      cout << "\t(N_cav >= " << round(tthreshold) << ")";
      thresholds[(unsigned)round(tthreshold)] = 0;
      tthreshold *= thrfactor;
   }
   cout << "\t\ti_cav(max)\n# \n";
   
   char lnin[256];
   ifstream* dord;
   if(integral)
   {
      unsigned fnlen = strlen(prefix) + 1;
      if(suffix_specified) fnlen += strlen(suffix);
      char dordname[fnlen];
      strcpy(dordname, prefix);
      if(suffix_specified) strcat(dordname, suffix);
      dord = new ifstream(dordname);
      if(dord->fail())
      {
         cout << "\nUnable to open \"" << dordname << "\".\n\n"
              << argv[0] << usage;
         exit(11);
      }
   }
   for(unsigned frame = first_frame; (!last_specified) || (last_frame >= frame); frame += jump)
   {
      if((integral) && (jump > 1))
      {
         for(unsigned i = 1; i < jump; i++)
         {
            if(dord->fail()) break;
            // dord->getline(lnin, 256);
            // int entries = atoi(lnin);
            int entries;
            *dord >> entries;
            // cout << "  [skipping " << entries << " entries]  ";
            dord->getline(lnin, 256);
            if((entries < 0) || (entries > 100000000))
            {
               cout << "\nError parsing number of entries:\n\t" << lnin << "\n";
               exit(16);
            }
            for(int j = 0; entries >= j; j++)
            {
               if(dord->fail()) { cout << "# \n# Skip failing.\n"; exit(16); }
               dord->getline(lnin, 256);
            }
         }
      }
      
      if(!integral)
      {
         bool opened = false;
         while(!opened && (digits < 13))
         {
            if(!digits_specified) digits++;
            unsigned fnlen = strlen(prefix) + digits + 1;
            
            if(suffix_specified) fnlen += strlen(suffix);
            char dordname[fnlen];
            strcpy(dordname, prefix);
            char framecode[digits+1];
            char fcformat[6];
            sprintf(fcformat, "%%.%ii", digits);
            sprintf(framecode, fcformat, frame);
            strcat(dordname, framecode);
            if(suffix_specified) strcat(dordname, suffix);
            
            dord = new ifstream(dordname);
            if(dord->fail())
            {
               if(digits_specified) break;
            }
            else opened = true;
         }
         digits_specified = true;
      }
      
      if(dord->fail()) break;
      // dord->getline(lnin, 256);
      // int entries = atoi(lnin);
      int entries;
      *dord >> entries;
      // cout << "  [expecting " << entries << " entries]  ";
      dord->getline(lnin, 256);
      if((entries < 0) || (entries > 100000000))
      {
         cout << "\nError parsing number of entries:\n\t" << lnin << "\n";
         exit(18);
      }
      if(dord->fail()) { cout << "\nSkip failing at comment line.\n"; exit(18); }
      dord->getline(lnin, 256);
      // cout << "\n\tskipping comment line [" << lnin << "]\n";
      
      Domain c(grid);
      string cavtype;
      double qabs[3];
      int qgrid[3];
      for(int j = 0; j < entries; j++)
      {
         if(dord->fail()) { cout << "\nSkip failing at cavity type token.\n"; exit(19); }
         *dord >> cavtype;
         for(unsigned d = 0; d < 3; d++)
         {
            if(dord->fail()) { cout << "\nInput failing.\n"; exit(20); }
            *dord >> qabs[d];
            if(qabs[d] > length)
            {
               cout << "\nCoordinate value " << qabs[d] << " exceeding specified box dimension (" << length << ").\n";
               exit(19);
            }
            qgrid[d] = (int)floor(qabs[d] * grid / length);
         }
         // cout << qgrid[0] << "  " << qgrid[1] << "  " << qgrid[2] << "\n";
         //
         c.insert(qgrid[0], qgrid[1], qgrid[2]);
      }
      cout << frame << "\t\t" << c.size() << "\t";
      
      c.detectClusters();
      unsigned maxsize = c.countClusters(&thresholds);
      
      map<unsigned, unsigned>::iterator threshit;
      for(threshit = thresholds.begin(); threshit != thresholds.end(); threshit++)
      {
         // cout << "\t[" << threshit->first << "] " << threshit->second;
         cout << "\t" << threshit->second;
      }
      cout << "\t\t" << maxsize << "\n";
      
      if(!integral)
      {
         dord->close();
         delete dord;
      }
   }
   if(integral)
   {
      dord->close();
      delete dord;
   }
   
   cout << "\n";
   return 0;
}
