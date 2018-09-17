/*!
 @file fluid.stft_pass_tilde
 
 Pass through STFT Max object
 
**/

#include "MaxNonRealTimeBase.hpp"

#include "ext.h"
#include "z_dsp.h"

#include "clients/rt/HPSSClient.hpp"

//Client class

//Data container
using fluid::FluidTensorView;

//using audio_client = BaseSTFTClient<double, double>;
//using audio_signal_wrapper = audio_client::AudioSignal;
//using scalar_signal_wrapper = audio_client::ScalarSignal;
//using signal_wrapper = audio_client::Signal<double>;

namespace fluid {
  namespace hpss {
    class HPSS_RT: public max::MaxNonRealTimeBase
    {
      using audio_client = HPSSClient<double, double>;
      using audio_signal_wrapper = audio_client::AudioSignal;
      using scalar_signal_wrapper = audio_client::ScalarSignal;
      using signal_wrapper = audio_client::Signal<double>;
      
      
      using SignalPointer = std::unique_ptr<signal_wrapper>;
    public:
      static void classInit(t_class* c, t_symbol* nameSpace, const char* classname)
      {
        makeAttributes<audio_client,HPSS_RT>(c);
        dspInit(c);
        addMethod<HPSS_RT,&HPSS_RT::dsp>(c);
      }
      

      
      
      
      HPSS_RT(t_symbol *s, long argc, t_atom *argv):
      fluid_obj(65536)
      {
        size_t offset = attr_args_offset(argc, argv);
        
        for(size_t i = 0,paramIdx = 0; i < offset; ++i)
        {
          switch(atom_gettype(argv+i))
          {
            case A_FLOAT:
            case A_LONG:
            {
              while((getParams()[paramIdx].getDescriptor().getType() != parameter::Type::Long
                    && getParams()[paramIdx].getDescriptor().getType() != parameter::Type::Float)
                    || !(getParams()[paramIdx].getDescriptor().instantiation() && !getParams()[paramIdx].getDescriptor().hasDefault()))
              {
                if(++paramIdx >= getParams().size())
                {
                  object_error(*this, "Could not parse arguments. Ran in trouble at argument %ld",i);
                  throw std::invalid_argument("");
                }
              }
              parameter::Instance& p = getParams()[paramIdx++];
              
              if(p.getDescriptor().getType() == parameter::Type::Long)
              {
                p.setLong(atom_getlong(argv + i));
              }
              else
              {
                p.setFloat(atom_getfloat(argv+i));
              }
              break;
            }
            case A_SYM:
            {
              while(getParams()[paramIdx].getDescriptor().getType() != parameter::Type::Buffer
                    || !(getParams()[paramIdx].getDescriptor().instantiation() && !getParams()[paramIdx].getDescriptor().hasDefault()))
              {
                if(++paramIdx >= getParams().size())
                {
                  object_error(*this, "Could not parse arguments. Ran in trouble at argument %ld",i);
                  throw std::invalid_argument("");;
                }
              }
              getParams()[paramIdx++].setBuffer(new max::MaxBufferAdaptor(*this, atom_getsym(argv + i)));
              break;
            }
          }
        }
        
        attr_args_process(*this, argc, argv + offset);
        
        for(auto&& p: getParams())
        {
          object_attr_setdisabled(*this, gensym(p.getDescriptor().getName().c_str()), p.getDescriptor().instantiation());
        }
        
        
        if(!fluid_obj.binaryP())
        {
          object_attr_setdisabled(*this,gensym("pthreshf1"),true);
          object_attr_setdisabled(*this,gensym("pthresha1"),true);
          object_attr_setdisabled(*this,gensym("pthreshf2"),true);
          object_attr_setdisabled(*this,gensym("pthresha2"),true);
        }
        if(!fluid_obj.binaryH())
        {
          object_attr_setdisabled(*this,gensym("hthreshf1"),true);
          object_attr_setdisabled(*this,gensym("hthresha1"),true);
          object_attr_setdisabled(*this,gensym("hthreshf2"),true);
          object_attr_setdisabled(*this,gensym("hthresha2"),true);
        }
        
        dspSetup(1);
        outlet_new(*this, "signal");
        outlet_new(*this, "signal");
        outlet_new(*this, "signal");
      }
      
      t_max_err param_set(t_object *attr, long argc, t_atom *argv, std::vector<parameter::Instance>& params)
      {
        
        struct Constraint{
          std::string param;
          std::function<bool(double, double)> condition;
        };
        
        static std::map<std::string, Constraint> paramConstraints{
          {"pthreshf1",{"pthreshf2", std::less<double>()}},
          {"hthreshf1",{"hthreshf2", std::less<double>()}},
          {"pthreshf2",{"pthreshf1", std::greater<double>()}},
          {"hthreshf2",{"hthreshf1", std::greater<double>()}}
        };
        
        
        t_symbol* attrname = (t_symbol *)object_method((t_object *)attr, gensym("getname"));
        double newval = atom_getfloat(argv);
        auto constraint = paramConstraints.find(attrname->s_name);
        
        if(constraint != paramConstraints.end())
        {
          double limit = parameter::lookupParam(constraint->second.param, getParams()).getFloat();
          
          if(!constraint->second.condition(newval,limit))
          {
            return 0 ;
          }
        }

        return MaxNonRealTimeBase::param_set(attr, argc, argv, params);
      }
      
      
      
      
      void dsp(t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
      {
//        no audio in left channel? Then no-op
        if(!count[0])
        {
          return;
        }
        
        //Delete old processor
        
        //Make new one with appropriate number of channels, pass in STFT params
        
        inputWrapper [0] = SignalPointer(new audio_signal_wrapper());
        
        outputWrapper[0] = SignalPointer(new audio_signal_wrapper());
        outputWrapper[1] = SignalPointer(new audio_signal_wrapper());
        outputWrapper[2] = SignalPointer(new audio_signal_wrapper());
//        fluid_obj.getParams()[0].setLong(pSize);
//        fluid_obj.getParams()[1].setLong(hSize);
//        fluid_obj.getParams()[2].setLong(window_size);
//        fluid_obj->getParams()[3].setLong(hop_size);
//        fluid_obj->getParams()[4].setLong(fft_size);
        
        
        bool isOK;
        std::string feedback;
        
        std::tie(isOK, feedback) = fluid_obj.sanityCheck();
        if(!isOK)
        {
          object_error(*this,feedback.c_str());
          return;
        }
        
        
        //TODO: I imagine some algorithms will need the sample rate in future as well
        fluid_obj.set_host_buffer_size(maxvectorsize);
        fluid_obj.reset();
        addPerform<HPSS_RT, &HPSS_RT::perform>(dsp64);
      }
      
      void perform(t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
      {
        inputWrapper[0]->set(ins[0], 0);
        outputWrapper[0]->set(outs[0],0);
        outputWrapper[1]->set(outs[1],0);
        outputWrapper[2]->set(outs[2],0);
        fluid_obj.do_process(inputWrapper.begin(),inputWrapper.end(), outputWrapper.begin(), outputWrapper.end(), sampleframes,1,3);
      }
      
      std::vector<parameter::Instance>& getParams()
      {
        return fluid_obj.getParams();
      }
      
    private:
      audio_client fluid_obj;
      std::array<SignalPointer,1> inputWrapper;
      std::array<SignalPointer,3> outputWrapper;
    };
  }
}

void ext_main(void *r)
{
  fluid::hpss::HPSS_RT::makeClass<fluid::hpss::HPSS_RT>(CLASS_BOX, "fluid.hpss~");
}