/*--------------------------------------------------------------------*/
/*--- Verrou: a FPU instrumentation tool.                          ---*/
/*--- Implementation of the software implementation of rounding    ---*/
/*--- mode switching.                                              ---*/
/*---                                            vr_roundingOp.hxx ---*/
/*--------------------------------------------------------------------*/

/*
   This file is part of Verrou, a FPU instrumentation tool.

   Copyright (C) 2014-2021 EDF
     F. Févotte     <francois.fevotte@edf.fr>
     B. Lathuilière <bruno.lathuiliere@edf.fr>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU Lesser General Public License is contained in the file COPYING.
*/


#pragma once
#include <limits>

//#ifndef LIBMATHINTERP
extern vr_RoundingMode ROUNDINGMODE;
//#else
//extern vr_RoundingMode ROUNDINGMODE;
//#endif

//#include "vr_fpRepr.hxx"
#include "vr_nextUlp.hxx"
#include "vr_isNan.hxx"

#include "vr_op.hxx"

template<class OP>
class RoundingNearest{
public:
  typedef typename OP::RealType RealType;
  typedef typename OP::PackArgs PackArgs;

  static inline RealType apply(const PackArgs& p){
    const RealType res=OP::nearestOp(p) ;
    OP::check(p,res);
    return res;
  } ;

};



template<class OP>
class RoundingFloat{
public:
  typedef typename OP::RealType RealType;
  typedef typename OP::PackArgs PackArgs;

  static inline RealType apply(const PackArgs& p){
    vr_roundFloat<typename PackArgs::RealType, PackArgs::nb> roundedArgs (p);
    const float res=(float) OP::nearestOp(roundedArgs.getPack()) ;
    return RealType(res);
  } ;

};




template<class OP>
class RoundingRandom{
public:
  typedef typename OP::RealType RealType;
  typedef typename OP::PackArgs PackArgs;

  static inline RealType apply(const PackArgs& p ){
    RealType res=OP::nearestOp(p);

    if (isNanInf<RealType> (res)){
      return res;
    }

    OP::check(p,res);
    const RealType signError=OP::sameSignOfError(p,res);
    if(signError==0.){
      return res;
    }else{
      const bool doNoChange = vr_rand_bool(&vr_rand);
      if(doNoChange){
	return res;
      }else{
	if(signError>0){
	  return nextAfter<RealType>(res);
	}else{
	  return nextPrev<RealType>(res);
	}
      }
    }
  } ;
};



template<class OP>
class RoundingAverage{
public:
  typedef typename OP::RealType RealType;
  typedef typename OP::PackArgs PackArgs;

  static inline RealType apply(const PackArgs& p){
    const RealType res=OP::nearestOp(p) ;

    if (isNanInf<RealType> (res)){
      return res;
    }

    OP::check(p,res);
    const RealType error=OP::error(p,res);
    if(error==0.){
      return res;
    }


    if(error>0){
      const RealType nextRes(nextAfter<RealType>(res));
      const RealType u(nextRes -res);
      const int s(1);
      const bool doNotChange = ((vr_rand_int(&vr_rand) * u)
				> (vr_rand_max() * s * error));
      if(doNotChange){
	return res;
      }else{
	return nextRes;
      }

    }
    if(error<0){
      const RealType prevRes(nextPrev<RealType>(res));
      const RealType u(res -prevRes);
      const int s(-1);
      const bool doNotChange = ((vr_rand_int(&vr_rand) * u)
				> (vr_rand_max() * s * error));
      if(doNotChange){
	return res;
      }else{
	return prevRes;
      }
    }
    return res; //Should not occur
  } ;
};



template<class OP>
class RoundingZero{
public:
  typedef typename OP::RealType RealType;
  typedef typename OP::PackArgs PackArgs;

  static inline RealType apply(const PackArgs& p){
    RealType res=OP::nearestOp(p) ;
    OP::check(p,res);
    const RealType signError=OP::sameSignOfError(p,res);

    if(isNanInf<RealType>(res)){
      if( (res!=std::numeric_limits<RealType>::infinity()) && (res!=-std::numeric_limits<RealType>::infinity())  ){
	return res;
      }else{
	if(OP::isInfNotSpecificToNearest(p)){
	  return res;
	}else{
	  if(res>0){
	    return std::numeric_limits<RealType>::max();
	  }else{
	    return -std::numeric_limits<RealType>::max();
	  }
	  }
      }
    }

    if( (signError>0 && res <0)||(signError<0 && res>0) ){
      return nextTowardZero<RealType>(res);
    }
    return res;
  } ;
};


template<class OP>
class RoundingUpward{
public:
  typedef typename OP::RealType RealType;
  typedef typename OP::PackArgs PackArgs;

  static inline RealType apply(const PackArgs& p){
    const RealType res=OP::nearestOp(p) ;
    OP::check(p,res);

    if(isNanInf<RealType>(res)){
      if(res!=-std::numeric_limits<RealType>::infinity()){
	return res;
      }else{
	if(OP::isInfNotSpecificToNearest(p)){
	  return res;
	}else{
	  return -std::numeric_limits<RealType>::max();
	}
      }
    }

    const RealType signError=OP::sameSignOfError(p,res);

    if(signError>0.){
      if(res==0.){
	return std::numeric_limits<RealType>::denorm_min();
      }
      if(res==-std::numeric_limits<RealType>::denorm_min()){
	return 0.;
      }
       return nextAfter<RealType>(res);
    }
    return res;
  } ;
};


template<class OP>
class RoundingDownward{
public:
  typedef typename OP::RealType RealType;
  typedef typename OP::PackArgs PackArgs;

  static inline RealType apply(const PackArgs& p){
    const RealType res=OP::nearestOp(p) ;
    OP::check(p,res);

    if(isNanInf<RealType>(res)){
      if(res!=std::numeric_limits<RealType>::infinity()){
	return res;
      }else{
	if(OP::isInfNotSpecificToNearest(p)){
	  return res;
	}else{
	  return std::numeric_limits<RealType>::max();
	}
      }
    }


    const RealType signError=OP::sameSignOfError(p,res);
    if(signError<0){
      if(res==0.){
	return -std::numeric_limits<RealType>::denorm_min();
      }
      if(res==std::numeric_limits<RealType>::denorm_min()){
	return 0.;
      }
      return nextPrev<RealType>(res);
    }
    return res;
  } ;
};



template<class OP>
class RoundingFarthest{
public:
  typedef typename OP::RealType RealType;
  typedef typename OP::PackArgs PackArgs;

  static inline RealType apply(const PackArgs& p){
    const RealType res=OP::nearestOp(p) ;
    if (isNanInf<RealType> (res)){
      return res;
    }

    OP::check(p,res);
    const RealType error=OP::error(p,res);
    if(error==0.){
      return res;
    }
    if(error>0){
      RealType newRes=nextAfter<RealType>(res);
      RealType ulp(newRes-res);
      if(2*error < ulp ){
	return newRes;
      }else{
	return res;
      }
    }else{//error<0
      RealType newRes=nextPrev<RealType>(res);
      RealType ulp(res-newRes);
      if(-2*error < ulp ){
	return newRes;
      }else{
	return res;
      }
    }
  }
};




#include "vr_op.hxx"

template<class OP>
class OpWithSelectedRoundingMode{
public:
  typedef typename OP::RealType RealType;
  typedef typename OP::PackArgs PackArgs;

  static inline void apply(const PackArgs& p, RealType* res, void* context){
    *res=applySeq(p,context);
#ifdef DEBUG_PRINT_OP
    print_debug(p,res);
#endif
    if (isNan(*res)) {
      vr_nanHandler();
    }
  }

#ifdef DEBUG_PRINT_OP
  static inline void print_debug(const PackArgs& p, const RealType* res){
    static const int nbParam= OP::PackArgs::nb;

    double args[nbParam];
    const double resDouble(*res);
    p.serialyzeDouble(args);
    if(vr_debug_print_op==NULL) return ;
    vr_debug_print_op(nbParam,OP::OpName(), args, &resDouble);
  }
#endif


  static inline RealType applySeq(const PackArgs& p, void* context){
    switch (ROUNDINGMODE) {
    case VR_NEAREST:
      return RoundingNearest<OP>::apply (p);
    case VR_UPWARD:
      return RoundingUpward<OP>::apply (p);
    case VR_DOWNWARD:
      return RoundingDownward<OP>::apply (p);
    case VR_ZERO:
      return RoundingZero<OP>::apply (p);
    case VR_RANDOM:
      return RoundingRandom<OP>::apply (p);
    case VR_AVERAGE:
      return RoundingAverage<OP>::apply (p);
    case VR_FARTHEST:
      return RoundingFarthest<OP>::apply (p);
    case VR_FLOAT:
      return RoundingFloat<OP>::apply (p);
    case VR_NATIVE:
      return RoundingNearest<OP>::apply (p);
    case VR_FTZ:
      vr_panicHandler("FTZ not implemented in backend_verrou");
    }

    return 0;
  }
};

//#endif
