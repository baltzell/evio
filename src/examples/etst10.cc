//  example program reads and dumps EVIO buffers

//  ejw, 20-sep-2011




#include "evioUtil.hxx"
#include "evioFileChannel.hxx"


using namespace evio;



//-------------------------------------------------------------------------------


int main(int argc, char **argv) {
  

  int bufferCount = 0;
  evioFileChannel *chan;
  vector<uint32_t>uiVec(10,100);
  float x[10] = {1,2,3,4,5,6,7,8,9,10};


  try {


    string dictXML = "<dict>\n"
      "<dictEntry name=\"fred\"   tag=\"1\" num=\"0\"/> \n"
      "<dictEntry name=\"wilma\"  tag=\"4\" num=\"11\"/>\n"
      "<dictEntry name=\"barney\" tag=\"5\" num=\"12\"/>\n"
      "<dictEntry name=\"betty\"  tag=\"6\" num=\"13\"/>\n"
      "</dict>\n";
    evioDictionary dict(dictXML);


    if(argc>1) {
      chan = new evioFileChannel(argv[1],"r");
    } else {
      chan = new evioFileChannel("fakeEvents.dat","r");
    }
    //    chan->setDictionary(dict);
    chan->open();
    cout << " <!-- xml boilerplate needs to go here -->\n\n<evio-data>\n"; 


    // loop over all buffers in file
    while(chan->read()) {


      // create event tree from channel contents
      evioDOMTree tree(chan);
      tree.setDictionary(dict);


//       tree.addBank("fred",uiVec);
//       tree.addBank("wilma",x,10);
//       tree << tree.createNode("barney",uiVec);
//       tree << tree.createNode("sally",uiVec);



      // dump tree
      // set configs
      evioToStringConfig config1(dict);
      //      config1.indentSize=10;
      //      evioToStringConfig config2(dictXML);

//       config.maxDepth=1;
//       config.noData=true;
//       config.xtod=true;
//      defaultToStringConfig.noData=true;



      bufferCount++;
      cout << "\n\n<!-- ===================== Buffer " << bufferCount << " contains " << (chan->getBuffer())[0] +1 << " words (" 
           << (chan->getBuffer())[0]*4+4 <<  " bytes) ===================== -->\n";
      cout << tree.toString();
      cout << endl << endl << "=================================================" << endl << endl;
      cout << tree.toString(config1);
      cout << "<!-- end buffer " << bufferCount << " -->\n\n";



//       cout << "getVectorUnique" << endl;
//       vector<double> *v = tree.getVectorUnique<double>(tagNumEquals("wilma",tree));
//       if(v!=NULL) {
//         cout << "vector size is " << v->size() << endl;
//       } else {
//         cout << "vector is null" << endl;
//       }


    }


    // done...close channel
    cout << "</evio-data>\n\n"; 
    chan->close();
    delete(chan);






  } catch (evioException e) {
    cerr << e.toString() << endl;

  } catch (...) {
    cerr << "?unknown exception" << endl;
  }
  
}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
