//  example of evio tree and bank creation


#include <vector>
#include "evioUtil.hxx"
using namespace evio;
using namespace std;


static evioFileChannel *chan = NULL;


int tag,num,len;
ContainerType cType;


vector<unsigned long> uVec1,uVec2;
double dBuf[100];
vector<long> lVec;
evioDOMNodeP parent;


//-------------------------------------------------------------------------------i


int main(int argc, char **argv) {
  
  // fill vectors
  for(int i=0; i<10; i++) uVec1.push_back(i),uVec2.push_back(2*i),dBuf[i]=i*10.0,lVec.push_back(100*i);



  try {
    chan = new evioFileChannel("ejw.dat","w");
    chan->open();
    


    // create tree with default bank type BANK and type BANK
    evioDOMNodeP root;
    evioDOMTree tree(root=evioDOMNode::createEvioDOMNode(tag=1, num=5));
    

    // create leaf node contining ulong and add to tree
    evioDOMNodeP ln1 = evioDOMNode::createEvioDOMNode(tag=2, num=6, uVec1); 
    tree.addBank(ln1);


    // create container node of BANKS and add to root node directly
    evioDOMNodeP cn2 = evioDOMNode::createEvioDOMNode(tag=3, num=7, cType=BANK);
    root->addNode(cn2);


    // add some leaf nodes to cn2
    evioDOMNodeP ln3 = evioDOMNode::createEvioDOMNode(tag=4, num=8, dBuf, 10);
    evioDOMNodeP ln4 = evioDOMNode::createEvioDOMNode(tag=5, num=9, lVec);
    cn2->addNode(ln3);
    cn2->addNode(ln4);

    
    // modify the data
    ln1->replace(uVec2);
    ln3->replace(dBuf,5);

    
    // write out tree
    chan->write(tree);
    chan->close();
    
  } catch (evioException e) {
    cerr << e.toString() << endl;
  }

  delete(chan);
  cout << "ejw.dat created" << endl;
}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
