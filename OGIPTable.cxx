//   Read the documentation to learn more about C++ code generator
//   versioning.
//	  %X% %Q% %Z% %W%
#include <CCfits/CCfits>
#include <Model/Component/Component.h>
#include <XSModel/Model/Component/OGIPTable/OGIPTableReadAll.h>
#include <XSModel/Model/Component/OGIPTable/OGIPTableDeferRead.h>
#include <XSModel/Parameter/TableModParam.h>

// UniqueEnergy
#include <XSModel/Model/UniqueEnergy.h>
// TableAccess
#include <XSModel/Model/Component/OGIPTable/TableAccess.h>
// ModParam
#include <XSModel/Parameter/ModParam.h>
// OGIPTable
#include <XSModel/Model/Component/OGIPTable/OGIPTable.h>

#include <Model/Component/AddTableComponent.h>
#include <Model/Component/MulTableComponent.h>
#include <XSFunctions/Utilities/xsFortran.h>
#include <Numerics/LinearInterp.h>
#include <XSUtil/Utils/XSstream.h>
#include <XSstreams.h>
#include <iostream>


// Class OGIPTable 
const string OGIPTable::s_MODLNAME = "MODLNAME";
const string OGIPTable::s_MODLUNIT = "MODLUNIT";
const string OGIPTable::s_REDSHIFT = "REDSHIFT";
const string OGIPTable::s_ADDMODEL = "ADDMODEL";
const string OGIPTable::s_HDUCLASS = "HDUCLASS";
const string OGIPTable::s_HDUCLAS1 = "HDUCLAS1";
const string OGIPTable::s_HDUVERS = "HDUVERS";
const string OGIPTable::s_NINTPARM = "NINTPARM";
const string OGIPTable::s_NADDPARM = "NADDPARM";
const string OGIPTable::s_NAME = "NAME";
const string OGIPTable::s_METHOD = "METHOD";
const string OGIPTable::s_INITIAL = "INITIAL";
const string OGIPTable::s_DELTA = "DELTA";
const string OGIPTable::s_MINIMUM = "MINIMUM";
const string OGIPTable::s_BOTTOM = "BOTTOM";
const string OGIPTable::s_TOP = "TOP";
const string OGIPTable::s_MAXIMUM = "MAXIMUM";
const string OGIPTable::s_NUMBVALS = "NUMBVALS";
const string OGIPTable::s_VALUE = "VALUE";
const string OGIPTable::s_HDUCLAS2 = "HDUCLAS2";
const string OGIPTable::s_ENERG_LO = "ENERG_LO";
const string OGIPTable::s_ENERG_HI = "ENERG_HI";
const string OGIPTable::s_PARAMVAL = "PARAMVAL";
const string OGIPTable::s_INTPSPEC = "INTPSPEC";
bool OGIPTable::s_init = false;
const string OGIPTable::s_UNITS = "UNITS";
const string OGIPTable::s_HDUVERS1 = "HDUVERS1";
const string OGIPTable::s_LOWELIMIT = "LOELIMIT";
const string OGIPTable::s_HIGHELIMIT = "HIELIMIT";
const Real OGIPTable::FUZZY = 1.E-06;
const long OGIPTable::s_MEM_THRESHOLD = 10000000;
string OGIPTable::s_prevFile = string("");
std::vector<std::string> OGIPTable::s_paramStrings;
std::vector<std::string> OGIPTable::s_energyStrings;
std::vector<std::string> OGIPTable::s_spectrumStrings;
std::vector<std::string> OGIPTable::s_hduPrimary;
std::vector<std::string> OGIPTable::s_hduNames;

OGIPTable::OGIPTable(const OGIPTable &right)
      : TableComponent(right),
        m_interParam(),
        m_exact(right.m_exact),
        m_startWeight(right.m_startWeight),
        m_endWeight(right.m_endWeight),
        m_startWeightBin(right.m_startWeightBin),
        m_endWeightBin(right.m_endWeightBin),
        m_recordNumbers(right.m_recordNumbers),
        m_bracket(right.m_bracket),
        m_addParam(),
	m_lowELim(right.m_lowELim),
	m_highELim(right.m_highELim),
        m_readStrategy(0)
{
   // m_interParam, m_addParam, and m_readStrategy will be set at a 
   // later point either in the setParamPointersFromCopy or read function.
}

OGIPTable::OGIPTable (const string& nameString, Component* p)
      : TableComponent(nameString,p),
        m_interParam(),
        m_exact(),
        m_startWeight(),
        m_endWeight(),
        m_startWeightBin(),
        m_endWeightBin(),
        m_recordNumbers(),
        m_bracket(),
        m_addParam(),
	m_lowELim(),
	m_highELim(),
        m_readStrategy(0)
{
  if (!s_init)
  {
        // initialize string arrays the first time this is called.

          s_paramStrings.push_back(s_NAME);
          s_paramStrings.push_back(s_METHOD);
          s_paramStrings.push_back(s_INITIAL);
          s_paramStrings.push_back(s_DELTA);
          s_paramStrings.push_back(s_MINIMUM);
          s_paramStrings.push_back(s_BOTTOM);
          s_paramStrings.push_back(s_TOP);
          s_paramStrings.push_back(s_MAXIMUM);
          s_paramStrings.push_back(s_NUMBVALS);
          s_paramStrings.push_back(s_VALUE);


          s_energyStrings.push_back(s_ENERG_LO);
          s_energyStrings.push_back(s_ENERG_HI);

          s_spectrumStrings.push_back(s_INTPSPEC);

          s_hduNames.push_back("PARAMETERS");
          s_hduNames.push_back("ENERGIES");


          s_hduPrimary.push_back(s_HDUCLASS);
          s_hduPrimary.push_back(s_HDUCLAS1);
          s_hduPrimary.push_back(s_HDUCLAS2);
          s_hduPrimary.push_back(s_HDUVERS);
          s_hduPrimary.push_back(s_ADDMODEL);
          s_hduPrimary.push_back(s_REDSHIFT);
          s_hduPrimary.push_back(s_MODLNAME);
          s_hduPrimary.push_back(s_MODLUNIT);
	  s_hduPrimary.push_back(s_LOWELIMIT);
	  s_hduPrimary.push_back(s_HIGHELIMIT);
          s_init = true;  
  }      
}


OGIPTable::~OGIPTable()
{
   delete m_readStrategy;
}


void OGIPTable::read (bool readSpectralData, Component* p, bool isAddRequest)
{
    string keywordErr;
    string columnErr;
    string extensionErr;
    try
    {
       using namespace CCfits;
       std::vector<std::vector<string> > hduKeys;

       hduKeys.push_back(s_paramStrings);
       hduKeys.push_back(s_energyStrings);
       extensionErr = "Primary";
       // Do NOT read in data from the SPECTRA extension at this time.
       // If this is a small matrix file, it may already be saved
       // in static arrays.  If it's a large matrix, we don't EVER
       // want to read the whole thing in.  We'll determine which way
       // to proceed further below.

       std::auto_ptr<FITS> readData(new FITS(filename(), Read, s_hduNames,
					         hduKeys, readSpectralData, s_hduPrimary));

       PHDU& primary = readData->pHDU();

       keywordErr = s_ADDMODEL;
       Keyword& add = primary.keyWord(s_ADDMODEL);
       bool isAdd (false);
       add.value(isAdd);

       // Owning component pointer p will be NULL if this is being
       // called from outside normal xspec channels - ie. if using
       // TableModel wrapper functions.  In that case check the
       // isAddRequest flag instead.
       if (p)
       {
          if ( isAdd && !dynamic_cast<AddTableComponent*>(p) )
          {
              string diag = " multiplicative model requested \nbut file ";
              diag += filename();
              diag += " contains additive model";    
              throw WrongTableType(diag);   
          } 
          else if (!isAdd && dynamic_cast<AddTableComponent*>(p) )
          {
              string diag = " additive model requested \nbut file ";
              diag += filename();
              diag += " contains multiplicative model";    
              throw WrongTableType(diag);   
          }
       }
       else
       {
          if (isAdd && !isAddRequest)
          {
              string diag = " multiplicative model requested \nbut file ";
              diag += filename();
              diag += " contains additive model";    
              throw WrongTableType(diag);   
          }
          else if (!isAdd && isAddRequest)
          {
              string diag = " additive model requested \nbut file ";
              diag += filename();
              diag += " contains multiplicative model";    
              throw WrongTableType(diag);   
          }          
       }

       parent(p);

       keywordErr = s_REDSHIFT;
       Keyword& red = primary.keyWord(s_REDSHIFT);
       bool redshift (false);
       red.value(redshift);
       keywordErr = s_MODLNAME;
       string modName; 
       primary.keyWord(s_MODLNAME).value(modName);
       if (modName.length() < 2)
       {
          string diag("Table Model MODLNAME keyword must contain a name with at least 2 characters.\n");
          throw YellowAlert(diag);
       }
       name(modName);
       keywordErr = s_LOWELIMIT;
       Real rValue;
       try {
	 primary.keyWord(s_LOWELIMIT).value(rValue);
       } catch(...) {
	 rValue = -1.0;
       }
       setLowELim(rValue);
       keywordErr = s_HIGHELIMIT;
       try {
	 primary.keyWord(s_HIGHELIMIT).value(rValue);
       } catch(...) {
	 rValue = -1.0;
       }
       setHighELim(rValue);

       // energy extension: number of energy points, energy high and low vectors.
       extensionErr = "ENERGIES";
       ExtHDU& energyExt = readData->extension("ENERGIES");

       columnErr = s_ENERG_LO;
       Column& eLow = energyExt.column(s_ENERG_LO);
       columnErr = s_ENERG_HI;
       Column& eHigh = energyExt.column(s_ENERG_HI);
       size_t Nenergy (readData->extension("ENERGIES").rows());

       RealArray enLow;
       RealArray enHigh;
       columnErr = s_ENERG_LO;
       eLow.read(enLow, 1, Nenergy);
       columnErr = s_ENERG_HI;
       eHigh.read(enHigh, 1, Nenergy);

       numEngVals(Nenergy);
       setEngLow(enLow);
       setEngHigh(enHigh);

       // parameter extension: each row in the file corresponds to a
       // parameter definition.
       extensionErr = "PARAMETERS";
       ExtHDU& paramsExt = readData->extension("PARAMETERS");
       int Nparams = paramsExt.rows();
       int nInterpParams(0);
       int nAdditionalParams(0);
       keywordErr = s_NADDPARM;
       paramsExt.readKey(s_NADDPARM, nAdditionalParams);
       keywordErr = s_NINTPARM;
       paramsExt.readKey(s_NINTPARM, nInterpParams);
       if (nInterpParams + nAdditionalParams != Nparams)
       {
          string errMsg("Table Model Error:\nIn PARAMETERS extension, NINTPARM + NADDPARM should = nRows\n");
          throw YellowAlert(errMsg);
       }

       numIntPar(nInterpParams);
       numAddPar(nAdditionalParams);

       m_exact.resize(nInterpParams,false);
       m_bracket.resize(nInterpParams,0);

       std::vector<string> names;
       std::vector<string> units;
       std::vector<RealArray> value;
       std::vector<Real> method, initial, delta, minimum, bottom, top, maximum;
       IntegerVector numbVals;

       columnErr = s_NAME;
       paramsExt.column(s_NAME).read(names, 1, Nparams);
       columnErr = s_METHOD;
       paramsExt.column(s_METHOD).read(method, 1, Nparams);
       columnErr = s_INITIAL;
       paramsExt.column(s_INITIAL).read(initial, 1, Nparams);
       columnErr = s_DELTA;
       paramsExt.column(s_DELTA).read(delta, 1, Nparams);
       columnErr = s_MINIMUM;
       paramsExt.column(s_MINIMUM).read(minimum, 1, Nparams);
       columnErr = s_BOTTOM;
       paramsExt.column(s_BOTTOM).read(bottom, 1, Nparams);
       columnErr = s_TOP;
       paramsExt.column(s_TOP).read(top, 1, Nparams);
       columnErr = s_MAXIMUM;
       paramsExt.column(s_MAXIMUM).read(maximum, 1, Nparams);
       columnErr = s_VALUE;
       try
       {
          paramsExt.column(s_VALUE).readArrays(value, 1, Nparams);
       }
       catch (CCfits::FitsException&)
       {
          // 2nd attempt, perhaps this is a scalar Value column.
          value.clear();
          std::vector<Real> tmpScalarCol;
          paramsExt.column(s_VALUE).read(tmpScalarCol, 1, Nparams);
          for (int i=0; i<Nparams; ++i)
          {
             RealArray tmpElem(tmpScalarCol[i],1);
             value.push_back(tmpElem);
          }
       }
       columnErr = s_NUMBVALS;
       paramsExt.column(s_NUMBVALS).read(numbVals, 1, Nparams);

       try
       {          
          paramsExt.column(s_UNITS).read(units, 1, Nparams);
       }
       catch ( Table::NoSuchColumn )
       { 
          // ignore this. 
          units.resize(Nparams,"");
       }

       if (!p)
       {
          // If there's no owning component -- meaning this is coming from a
          // TableModel wrapper function, then we have 2 extra things to
          // be aware of:
          //   1.  Can't assume m_interParam, m_addParam, and TableComponent's
          //       m_params are all empty, since this object is persistent
          //       and may go through many read calls.  Therefore we must clear
          //       those arrays here.
          //   2.  TableComponent's m_params is the OWNER of these pointers,
          //       since there is no parent Component and these are not registered
          //       in XSPEC's global containers.
          m_interParam.clear();
          m_addParam.clear();
          std::vector<Parameter*>& owningPointers = params();
          for (size_t i=0; i<owningPointers.size(); ++i)
             delete owningPointers[i];
          owningPointers.clear();

       }

       // the following loop works both for the cases of all the interpolation
       // parameters followed by all the additive parameters or them mixed 
       // amongst each other with additive parameters indicated by method=-1 or
       // no tabulated parameter values.

       for (int i = 0; i < Nparams; ++i)
       {

	 if (method[i] >= 0 && m_interParam.size() < (size_t)nInterpParams 
	     && numbVals[i] > 0)
	 {
	   // case of an interpolated parameter
	   std::auto_ptr<TableModParam>  modParam (new TableModParam(names[i], 0, 
					 initial[i] , delta[i], maximum[i],
			                 minimum[i], top[i], bottom[i],units[i]));

	   modParam->numVals(numbVals[i]);
	   RealArray ivals(0.,numbVals[i]);
	   std::copy(&value[i][0],&value[i][0]+numbVals[i],&ivals[0]);
	   modParam->setTabValue(ivals);
	   modParam->logInterp(method[i]);
	   params().push_back(modParam.release());
	   m_interParam.push_back(static_cast<TableModParam*>(params().back()));
	 } 
	 else 
	 {
	   // case of an additive parameter
	   std::auto_ptr<ModParam> modPar(new ModParam(names[i], 0, initial[i],
                        delta[i], maximum[i], minimum[i], top[i], bottom[i],
                        units[i]));
	   params().push_back(modPar.release());
	   m_addParam.push_back(static_cast<ModParam*>(params().back()));
	 }
       }

       if ( redshift )
       {
           // construct a new modparam 
           params().push_back(new ModParam(string("z"),0,0.,-0.001,5.,0.,5.,0.));
       }

       // spectra extension contains the interpolant.
       // the table is defined, for each energy, at Pi Npar_i points,
       // where Npar_i is the number of points at which the ith
       // interpolating [intPar] parameter is defined.
       // each additive [addPar] parameter adds a table of the same size. 
       extensionErr = "SPECTRA";

       // Still not reading in the interp data rows yet, just adding
       // the extension.
       readData->read(string("SPECTRA"), false); 
       ExtHDU& spectraExt = readData->extension("SPECTRA");
       int spectrumRows = spectraExt.rows();
       if (spectrumRows < 1)
       {
          throw YellowAlert("No rows found in spectrum extension.\n");
       }
       long grandTotal = static_cast<long>(spectrumRows)*Nenergy*
                                (1 + nAdditionalParams);
       const bool isLarge = grandTotal > s_MEM_THRESHOLD;

       string absPathFile;
       if (filename().find_first_of("/") != 0)
       {
          absPathFile = XSutility::getRunPath();
          absPathFile += "/";
       }
       absPathFile += filename();
       const bool isSameAsPrev = (absPathFile == s_prevFile);
       if (isLarge)
       {
          // m_readStrategy may have been previously set, ie.
          // when coming from a TableModel wrapper function.
          delete m_readStrategy;
          m_readStrategy = new OGIPTableDeferRead();
       }
       else
       {
          delete m_readStrategy;
          m_readStrategy = new OGIPTableReadAll();
          if (!isSameAsPrev)
             s_prevFile = absPathFile;
       }

       columnErr = s_INTPSPEC;
       m_readStrategy->initialRead(spectraExt, Nenergy, nAdditionalParams,
                        isSameAsPrev);
    }
    catch ( WrongTableType )
    {
        string diag(" : ");
        diag += filename() + "\n";
        throw YellowAlert(diag);       

    }
    catch (CCfits::HDU::NoSuchKeyword&)
    {
       string diag("Table model missing keyword: ");
       diag += keywordErr + "\n";
       throw YellowAlert(diag);
    }
    catch (CCfits::Table::NoSuchColumn&)
    {
       string diag("While attempting to read table model column: ");
       diag += columnErr + "\n";
       throw YellowAlert(diag);
    }
    catch (CCfits::FITS::NoSuchHDU&)
    {
       string diag("While attempting to open table model ");
       diag += extensionErr + " hdu.\n";
       throw YellowAlert(diag);
    }
    catch ( CCfits::FitsException&)
    {
        string diag(" table model file read failed: ");
        diag += filename();
        throw YellowAlert(diag);       
    }
}

OGIPTable* OGIPTable::clone (Component* parent) const
{
   OGIPTable* newTable = new OGIPTable(*this);
   newTable->parent(parent);
   newTable->setParamPointersFromCopy();
   if (m_readStrategy)
   {
      newTable->m_readStrategy = m_readStrategy->clone();
   }
   return newTable;
}

bool OGIPTable::formatCheck (const string& fileName)
{
    using namespace CCfits;
    bool result (false);
    string hduClass("");
    string hduClas1("");
    string hduVers("");


    //strip leading and trailing white space?
    try
    {
            std::auto_ptr<FITS>  fitsFile(new FITS(fileName));

            PHDU& bintable = fitsFile->pHDU();
            bintable.readKey(s_HDUCLASS, hduClass);
            bintable.readKey(s_HDUCLAS1, hduClas1);
            try
            {
                    bintable.readKey(s_HDUVERS, hduVers);
            }
            catch ( HDU::NoSuchKeyword ) 
            {
                    // band-aid for files that don't quite meet
                    // the standard
                    string vers1(s_HDUVERS1);
                    bintable.readKey(vers1,hduVers);  
            }     
    }
    catch ( FitsException& ) 
    {
                // any problem with reading required keywords,
                // opening file, etc. etc.
                // do nothing: keep result as false.
    }


    result = hduClass.substr(0,4) == "OGIP" 
                        &&  hduClas1 == "XSPEC TABLE MODEL" 
                        && hduVers.substr(0,2) == "1.";

    return result;
}

void OGIPTable::getInterpolantIndices ()
{

   size_t Ninter = m_interParam.size();
   if ( Ninter != static_cast<size_t>(numIntPar())) 
   {
      throw RedAlert("redundancy check for # of interpolated params");
   }
   // now, test for limits.
   size_t totalBlockSize(1);
   std::vector<IntegerVector> prepRecordNumbers(Ninter);
   for (size_t j= 0; j < Ninter; ++j)
   {
      Real value = m_interParam[j]->value();
      const RealArray& tabValue = m_interParam[j]->tabValue();
      size_t N = tabValue.size();
      // accumulate product of numbers of parameter values,
      // will be used to calculate table offsets below.
      totalBlockSize *= N;
      size_t kExact (0);
      if ( N > 1 )
      {
         // Condition for out-of-bounds test must be consistent with
         // the test for exactness below.  Note that tabValue's original
         // input comes from floats in a FITS file, not doubles.
         const Real fuzz = (value == 0.0) ? 0.0 : FUZZY;
         const Real magnitude = (value == 0.0) ? 1.0 : std::abs(value);
         if ((tabValue[0] - value)/magnitude > fuzz)
	 {
	   std::ostringstream diag;
	   diag << "  " << m_interParam[j]->name() << " = " << value 
		<< " which is < " << tabValue[0];
            throw ParameterRangeError(diag.str());
	 } 
         else if ((value - tabValue[N-1])/magnitude > fuzz)
	 {
	   std::ostringstream diag;
	   diag << "  " << m_interParam[j]->name() << " = " << value 
		<< " which is > " << tabValue[N-1];
            throw ParameterRangeError(diag.str());
         }
         else
         {
            for (; kExact < N-1; ++kExact)
            {
	      if ( (m_exact[j] 
		    = (std::abs((value - tabValue[kExact])/magnitude) <= fuzz)) )
                       break;
            }       
         }

      }
      else
      {
         m_exact[j] = true;
      }

      if (m_exact[j])
      {
         // just store this here for now, will resize
         // correctly later.
         prepRecordNumbers[j].resize(1,kExact);
      }
      else
      {
         size_t n (1);
         // 2^(j+1) records needed.
         for ( size_t l = 0; l <= j ; ++l ) if (!m_exact[l]) n *= 2; 
         prepRecordNumbers[j].resize(n,0);
      }
   }

   IntegerVector blockOffset(Ninter,1);
   blockOffset[0] = totalBlockSize;

   size_t jj(0);
   do 
   {
      totalBlockSize /= m_interParam[jj]->tabValue().size(); 
      blockOffset[jj] = totalBlockSize;    
      ++jj;  
   } while ( jj < Ninter - 1);

   for (size_t j = 0; j < Ninter; ++ j)
   {
      Real value = m_interParam[j]->value();
      const RealArray& tabValue = m_interParam[j]->tabValue();
      if ( !m_exact[j] )
      {
         // m_bracket[j] is the arraypoint in interpValues below the target
         // value, which is therefore straddled by (m_bracket[j],m_bracket[j]+1).
         // if the range is exceed, perform constant extrapolation.
         XSutility::find(tabValue,value,m_bracket[j]);
         if ( m_bracket[j] >= static_cast<int>(tabValue.size() - 1))
         {
            m_exact[j] = true;
            prepRecordNumbers[j][0] = tabValue.size() - 1;   
            for ( size_t l  = j ;  l < Ninter; ++l)
            {
               prepRecordNumbers[l].resize(prepRecordNumbers[l].size()/2,0);   
            }
         }
         else if ( m_bracket[j] < 0)
         {
            m_exact[j] = true;
            prepRecordNumbers[j][0] = 0;   
            for ( size_t l  = j ;  l < Ninter; ++l)
            {
               prepRecordNumbers[l].resize(prepRecordNumbers[l].size()/2,0);   
            }      
         }
         else
         {
            if ( j == 0 )
            {

               prepRecordNumbers[0][0] = blockOffset[0]*(m_bracket[0]);   
               prepRecordNumbers[0][1] = blockOffset[0]*(m_bracket[0] + 1);  
            }
            else
            {
               IntegerVector& previous = prepRecordNumbers[j-1];
               size_t MP = previous.size();
               for (size_t k = 0; k < MP; ++k)
               {
                   prepRecordNumbers[j][2*k] 
                           = previous[k]  + blockOffset[j]*(m_bracket[j]);
                   prepRecordNumbers[j][2*k + 1] 
                           = previous[k]  + blockOffset[j]*(m_bracket[j] + 1);

               }
            }

         }
      }  

      if (m_exact[j]) 
      {
         int kExact = prepRecordNumbers[j][0];
         if ( j == 0 )
         {
            // this will evaluate correctly to the #of blocks
            // before the one containing the record of interest
            // because kExact is 0 based. 
            prepRecordNumbers[j][0] = blockOffset[0]*kExact;
         }
         else
         {
            IntegerVector& previous = prepRecordNumbers[j-1];
            prepRecordNumbers[j].resize(previous.size());
            size_t MP = previous.size();
            for (size_t k = 0; k < MP; ++k)
            {
               prepRecordNumbers[j][k] = previous[k] + 
                                               blockOffset[j]*kExact;
            }
         }
      }     
   }
   m_recordNumbers = prepRecordNumbers[Ninter - 1];
}

std::pair<Real,Real> OGIPTable::energyPoint (size_t energyIndex, const RealArray& fraction, TableValues& workspace)
{
   std::pair<Real, Real> output (0.,0.);  
   // at energy energyIndex, gather up all the interpolant information
   // into arrays of size records.size() do the interpolation and output
   // interpolant and error in the return value 
   const size_t NP(m_interParam.size());
   const size_t NR(m_recordNumbers.size());
   const size_t NA(numAddPar());

   Real* spectrumEntries = workspace.m_interpValues[0];
   m_readStrategy->getSpectrumEntries(energyIndex, spectrumEntries);

   Real* varianceEntries = 0;
   if (m_readStrategy->isError())
   { 
      varianceEntries = workspace.m_interpValueError[0];
      m_readStrategy->getVarianceEntries(energyIndex, varianceEntries);
   }
   // accumulate additional parameter effects
   for ( size_t k = 0; k < NA; ++k)
   {
      Parameter* p = m_addParam[k];
      Real addParVal = p->value();
      Real* addSpCol = workspace.m_addSpectra[k][0];
      m_readStrategy->getAddSpectra(energyIndex, k, addSpCol);
      for ( size_t j = 0; j < NR; ++j)
      {
         spectrumEntries[j] +=  addParVal*addSpCol[j];      
      }

      if (m_readStrategy->isAddParamError()[k])
      {
         Real* addSpErrCol = workspace.m_addSpectraError[k][0];
         m_readStrategy->getAddVariance(energyIndex, k, addSpErrCol);
         for ( size_t j = 0; j < NR; ++j)
         {
            varianceEntries[j] +=  addParVal*addSpErrCol[j];      
         }
      }
   }

   if ( NR == 1 )
   {
      // all parameters are "exact"
      output.first = spectrumEntries[0];
      if ( m_readStrategy->isError() ) output.second = varianceEntries[0];

   }
   else
   {
      size_t nd = NR;
      Real* reducedSp = new Real[nd];
      Real* reducedVar = new Real[nd];
      for ( int j = NP - 1; j >= 0; --j )
      {
         Real factor = fraction[j];
         if ( !m_exact[j] )
         {
            nd /= 2;
            for ( size_t k = 0; k < nd; ++k )
            {
               const size_t k2 = 2*k;
               reducedSp[k] = spectrumEntries[k2]  + 
                       factor*(spectrumEntries[k2+1] - spectrumEntries[k2] ) ;
               if ( m_readStrategy->isError() )
               {
                  reducedVar[k] = varianceEntries[k2] +  
                          factor*(varianceEntries[k2+1] - varianceEntries[k2]);       
               }  
            }
         }
         else
         {
            for ( size_t k = 0; k < nd; ++k ) reducedSp[k] = spectrumEntries[k];
            if ( m_readStrategy->isError() )
            {
               for ( size_t k = 0; k < nd; ++k ) 
               {
                  reducedVar[k] = varianceEntries[k];
               }
            }
         }
         for (size_t i=0; i<nd; ++i)
            spectrumEntries[i] = reducedSp[i];
         if (m_readStrategy->isError())
            for (size_t i=0; i<nd; ++i) 
               varianceEntries[i] = reducedVar[i];

      }
      delete [] reducedSp;
      delete [] reducedVar;

      if (nd != 1 ) 
      {
         throw RedAlert("Programming error in OGIPTable::energyPoint");
      }
      output.first = spectrumEntries[0];
      if (m_readStrategy->isError()) output.second = varianceEntries[0];
   }
   return output;
}

void OGIPTable::getInterpolant (RealArray& spectrum, RealArray& variance)
{

  getInterpolantIndices();  

  const size_t N (numEngVals());
  spectrum.resize(N);
  if (m_readStrategy->isError()) variance.resize(N);
  m_readStrategy->initialAccessRows(m_recordNumbers, static_cast<size_t>(numAddPar()));

  RealArray fraction(0.0, m_interParam.size());
  for (size_t i=0; i<fraction.size(); ++i)
  {
     if (!m_exact[i])
     {
        const TableModParam* param = m_interParam[i];
        const RealArray& tParValues = param->tabValue();
        const Real parValue = param->value();
        Real x1 = tParValues[m_bracket[i]];
        Real x2 = tParValues[m_bracket[i]+1];
        if (!param->logInterp())
        {
           fraction[i] = (parValue - x1)/(x2 - x1);
        }
        else
        {
           // we know x1 < parVal < x2.
           // now, we ought to check that x1,x2 > 0 earlier
           // than this point!
           fraction[i] = log(parValue/x1)/log(x2/x1);           
        }
     }
  }

  // Create a workspace to gather one energy column's worth of the
  // various interp arrays.  We do this here once to avoid having the 
  // energyPoint function allocate/deallocate these arrays N times.
  TableValues workspace;
  workspace.m_nSpecRows = m_recordNumbers.size();
  workspace.m_interpValues.resize(1,0);
  workspace.m_interpValues[0] = new Real[workspace.m_nSpecRows];
  if (m_readStrategy->isError())
  {
     workspace.m_interpValueError.resize(1,0);
     workspace.m_interpValueError[0] = new Real[workspace.m_nSpecRows];     
  }
  const size_t nAddPar = numAddPar();
  workspace.m_addSpectra.resize(nAddPar);
  workspace.m_addSpectraError.resize(nAddPar);
  for (size_t i=0; i<nAddPar; ++i)
  {
     workspace.m_addSpectra[i].resize(1,0);
     workspace.m_addSpectra[i][0] = new Real[workspace.m_nSpecRows];
     if (m_readStrategy->isAddParamError()[i])
     {
        workspace.m_addSpectraError[i].resize(1,0);
        workspace.m_addSpectraError[i][0] = new Real[workspace.m_nSpecRows];
     }
  }

  for ( size_t j = 0; j < N; ++j)
  {
          std::pair<Real,Real> point ( energyPoint(j, fraction, workspace) );
          spectrum[j] = point.first;
          if (m_readStrategy->isError() ) variance[j] = point.second;
  }

//  tcout << " Interpolant \n";
//  for (size_t j = 0; j < N ; ++j)
//  {
//          tcout << j << " " << engLow(j) << " " << spectrum[j] << '\n';
//  }
//  tcout << std::flush;
}

void OGIPTable::energyWeights (const UniqueEnergy* uniqueEng)
{
  // produce weights for elements of the energy array. 
  // these are used to rebin the componentSpectrum - which is on the
  // energy grid of the tabulated data - into the energy array of the model
  // component.   

  // reworking of the inibin.f function of xspec11.
  const RealArray& energyArray = uniqueEng->energy();
  size_t Nout ( energyArray.size() - 1);

  IntegerVector& startBin = m_startWeightBin[uniqueEng];
  IntegerVector& endBin   = m_endWeightBin[uniqueEng];        
  RealArray& startWeight = m_startWeight[uniqueEng];
  RealArray& endWeight   = m_endWeight[uniqueEng];                

  Real z (0);

  XSutility::MatchPtrName<Parameter> matchName;
  std::vector<Parameter*>::iterator r =
                   std::find_if(params().begin(),params().end(),bind2nd(matchName,"z"));

  if ( r != params().end() ) z = (*r)->value();

  if (z <= -1.0)
  {
     string err("Cannot calculate table model with redshift z <= -1.0\n");
     throw YellowAlert(err);
  }
  bool redshift ( z != 0 );
  Real zf (1.);
  if ( redshift ) zf /= (1+z);

  size_t Nin  ( engLow().size());
  RealArray enArray(0.,Nin+1);
  for (size_t k = 0; k < Nin;++k) enArray[k] = engLow(k);
  enArray[Nin] = engHigh(Nin-1);
  RealArray observedEnergy ( enArray*zf ); 

  size_t inputBin = 0;
  size_t outputBin = 0;

  // DO NOT stream output to tcout.  This code may be accessed through xsmtbl and xsatbl 
  // by users linking the models library to their own programs. 


  if (!Numerics::Rebin::findFirstBins(observedEnergy, energyArray, FUZZY, 
                inputBin, outputBin))
  {
     // Deliberately not throwing here for (at least) 2 reasons:
     // Data command doesn't expect its Notify call to throw, and therefore
     // isn't catching anything at that point.  Also it's still possible
     // a calculation will be performed on this model, in which case it
     // expects to have valid bin and weight arrays (doing "plot model"
     // after throwing from here caused a seg-fault).
    std::ostringstream oss;  
    oss	  << "\n There is no model information available in the requested observed frame "
	  << " energy range ( " << energyArray[0] << "," << energyArray[Nout] << ")\n"
	  << " the model rest frame energy range is " << enArray[0] << " to " << enArray[Nin] << '\n'
	  << " redshift requested is " << z << "\n";
     xs_write(const_cast<char*>(oss.str().c_str()),10);            
     size_t Nout (energyArray.size() - 1);
     startWeight.resize(Nout,0);
     endWeight.resize(Nout,0);
     startBin.clear();
     endBin.clear();
     startBin.resize(Nout,0);
     endBin.resize(Nout,0);
  }
  else
  {
     if (outputBin > 0) 
     {
       std::ostringstream oss;
	    oss << "\n No model information is available for observed frame energies below "
	     << std::setw(14) << observedEnergy[0] 
	     << " so model is set to zero at these energies\n";
       xs_write(const_cast<char*>(oss.str().c_str()), 15);
     }

     Numerics::Rebin::initializeBins(observedEnergy, energyArray, FUZZY, inputBin, outputBin,
                            startBin, endBin, startWeight, endWeight);            
     if ( inputBin >= Nin )
     {
       std::ostringstream oss;
       oss << "\nNo model information is available for observed frame energies \n"
	     << "above " << observedEnergy[Nin] 
	     << " so model is taken as zero in that range\n";
       xs_write(const_cast<char*>(oss.str().c_str()), 15);
     }
  }
}

void OGIPTable::clearArrays (const std::set<UniqueEnergy*>& currentEngs)
{

  std::map<const UniqueEnergy*,IntegerVector>::iterator m (m_startWeightBin.begin());
  std::map<const UniqueEnergy*,IntegerVector>::iterator mEnd (m_startWeightBin.end());

  while ( m != mEnd )
  {
     // Casting away a const is generally not a good thing, but
     // it's only to allow m->first to be used as a key.
     // It doesn't actually modify what nm points to.
       UniqueEnergy* nm = const_cast<UniqueEnergy*>(m->first);
       ++m;
       if (currentEngs.find(nm) == currentEngs.end())
       {
           m_startWeightBin.erase(nm);           
           m_endWeightBin.erase(nm);           
           m_startWeight.erase(nm);           
           m_endWeight.erase(nm);           
       }         
  }
}

void OGIPTable::rebinComponent (const UniqueEnergy* uniqueEng, const RealArray& inputArray, RealArray& outputArray, bool variance)
{

  IntegerVector& startBin   = m_startWeightBin[uniqueEng];     
  IntegerVector& endBin     = m_endWeightBin[uniqueEng];     
  RealArray endWeight (m_endWeight[uniqueEng]);     
  RealArray startWeight (m_startWeight[uniqueEng]);     

  if ( variance )
  {
        startWeight *= m_startWeight[uniqueEng];
        endWeight   *= m_endWeight[uniqueEng];       
  }

  Real lowValue = lowELim();
  Real highValue = highELim();
  if ( lowValue < 0.0 ) lowValue = 0.0;
  if ( highValue < 0.0 ) highValue = 0.0;

  Numerics::Rebin::rebin(inputArray, startBin, endBin, startWeight, endWeight, outputArray, lowValue, highValue);
}

void OGIPTable::interpolateComponent (const UniqueEnergy* uniqueEng, const RealArray& inputArray, RealArray& outputArray, bool exponential)
{
  IntegerVector& startBin   = m_startWeightBin[uniqueEng];     
  IntegerVector& endBin     = m_endWeightBin[uniqueEng];     
  RealArray& endWeight     = m_endWeight[uniqueEng];     
  RealArray& startWeight   = m_startWeight[uniqueEng];     

  Real lowValue = lowELim();
  Real highValue = highELim();
  if ( lowValue < 0.0 ) lowValue = 1.0;
  if ( highValue < 0.0 ) highValue = 1.0;

  Numerics::Rebin::interpolate(inputArray, startBin, endBin, startWeight, 
			       endWeight, outputArray, exponential, lowValue, highValue);
}

void OGIPTable::setParamPointersFromCopy ()
{
   // If TableComponent's parent hasn't been set yet (ie. if copying
   // from a prototype), nothing will be done in base class
   // setParamPointersFromCopy, and by extension nothing will be done
   // in here.  This is for copying active model objects, ie. when
   // adding data in a new data group.
   TableComponent::setParamPointersFromCopy();
   std::vector<Parameter*>::const_iterator itPar = params().begin();
   std::vector<Parameter*>::const_iterator itEnd = params().end();
   while (itPar != itEnd)
   {
      // I wish there was a better way to do this.
      TableModParam* tablePar = dynamic_cast<TableModParam*>(*itPar);
      if (tablePar)
      {
         m_interParam.push_back(tablePar);
      }
      else
      {
         m_addParam.push_back(static_cast<ModParam*>(*itPar));
      }
      ++itPar;
   }
}

// Additional Declarations

