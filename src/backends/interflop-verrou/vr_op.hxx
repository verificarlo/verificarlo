
/*--------------------------------------------------------------------*/
/*--- Verrou: a FPU instrumentation tool.                          ---*/
/*--- Implementation of error estimation for all FP operations     ---*/
/*---                                                    vr_op.hxx ---*/
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



template<class> struct realTypeHelper;

template<>
struct realTypeHelper<float>{
  typedef float SimdBasicType;
  static const int SimdLength=1;
};



template<>
struct realTypeHelper<double>{
  typedef double SimdBasicType;
  static const int SimdLength=1;
};

template<>
struct realTypeHelper<long double>{
  typedef long double SimdBasicType;
  static const int SimdLength=1;
};


template<class REALTYPESIMD>
struct realTypeHelper<const REALTYPESIMD>{
  typedef typename realTypeHelper<REALTYPESIMD>::SimdBasicType SimdBasicType;
  static const int SimdLength=realTypeHelper<REALTYPESIMD>::SimdLength;
};




template<class REALTYPE, int NB>
struct vr_packArg;


template<class REALTYPE>
struct vr_packArg<REALTYPE,1>{
  static const int nb= 1;
  typedef REALTYPE RealType;
  typedef typename realTypeHelper<REALTYPE>::SimdBasicType SimdBasicType;
  typedef vr_packArg<SimdBasicType,1> SubPack;
  
  inline vr_packArg(const RealType& v1):arg1(v1)
  {
  };

  inline const SubPack& getSubPack(int I)const{
    return SubPack(arg1[I]);
  }

  inline void serialyzeDouble(double* res)const{
    res[0]=(double)arg1;
  }

  inline bool isOneArgNanInf()const{
    return isNanInf<RealType>(arg1);
  }
  
  const RealType& arg1;


};


template<class REALTYPE>
struct vr_packArg<REALTYPE,2>{
  static const int nb= 2;
  typedef REALTYPE RealType;
  typedef typename realTypeHelper<REALTYPE>::SimdBasicType SimdBasicType;
  typedef vr_packArg<SimdBasicType,2> SubPack;
  
  vr_packArg(const RealType& v1,const RealType& v2):arg1(v1),arg2(v2)
  {
  };

  inline const SubPack getSubPack(int I)const{
    return SubPack(arg1[I],arg2[I]);
  }
  
  inline void serialyzeDouble(double* res)const{
    res[0]=(double)arg1;
    res[1]=(double)arg2;
  }

  inline bool isOneArgNanInf()const{
    return (isNanInf<RealType>(arg1) || isNanInf<RealType>(arg2));
  }

  
  const RealType& arg1;
  const RealType& arg2;
};


template<class REALTYPE>
struct vr_packArg<REALTYPE,3>{
  static const int nb= 3;
  typedef REALTYPE RealType;
  typedef typename realTypeHelper<REALTYPE>::SimdBasicType SimdBasicType;
  typedef vr_packArg<SimdBasicType,3> SubPack;
  
  vr_packArg(const RealType& v1,const RealType& v2,const RealType& v3):arg1(v1),arg2(v2),arg3(v3){
  };

  inline const SubPack& getSubPack(int I)const{
    return SubPack(arg1[I],arg2[I],arg3[I]);
  }
  
  inline void serialyzeDouble(double* res)const{
    res[0]=(double)arg1;
    res[1]=(double)arg2;
    res[2]=(double)arg3;
  }

  inline bool isOneArgNanInf()const{
    return (isNanInf<RealType>(arg1) || isNanInf<RealType>(arg2) || isNanInf<RealType>(arg3) );
  }

  
  const RealType& arg1;
  const RealType& arg2;
  const RealType& arg3;
};


template<class REALTYPE, int NB>
class vr_roundFloat;


template<class REALTYPE>
struct vr_roundFloat<REALTYPE, 1>{
  vr_roundFloat(const vr_packArg<REALTYPE,1>& p): arg1(REALTYPE(float(p.arg1))){
  }
  vr_packArg<REALTYPE,1> getPack()const{
    return vr_packArg<REALTYPE,1>(arg1);
  }
  const REALTYPE arg1;
};


template<class REALTYPE>
struct vr_roundFloat<REALTYPE, 2>{
  vr_roundFloat(const vr_packArg<REALTYPE,2>& p): arg1(REALTYPE(float(p.arg1 ))),
						  arg2(REALTYPE(float(p.arg2 ))){
  }
  vr_packArg<REALTYPE,2> getPack()const{
    return vr_packArg<REALTYPE,2>(arg1,arg2);
  }
  const REALTYPE arg1;
  const REALTYPE arg2;
};

template<class REALTYPE>
struct vr_roundFloat<REALTYPE, 3>{
  vr_roundFloat(const vr_packArg<REALTYPE,3>& p): arg1(REALTYPE(float(p.arg1 ))),
						  arg2(REALTYPE(float(p.arg2 ))),
						  arg3(REALTYPE(float(p.arg3 ))){
  }
  vr_packArg<REALTYPE,3> getPack()const{
    return vr_packArg<REALTYPE,3>(arg1,arg2,arg3);
  }
  const REALTYPE arg1;
  const REALTYPE arg2;
  const REALTYPE arg3;
};





template<typename REAL>
class AddOp{
public:
  typedef REAL RealType;
  typedef vr_packArg<RealType,2> PackArgs;
#ifdef DEBUG_PRINT_OP
  static const char* OpName(){return "add";}
#endif

  static inline RealType nearestOp (const PackArgs&  p) {
    const RealType & a(p.arg1);
    const RealType & b(p.arg2);
    return a+b;
  }

  static inline RealType error (const PackArgs& p, const RealType& x) {
    const RealType & a(p.arg1);
    const RealType & b(p.arg2);
    const RealType z=x-a;
    return ((a-(x-z)) + (b-z)); //algo TwoSum
  }

  static inline RealType sameSignOfError (const PackArgs& p,const RealType& c) {
    return AddOp<RealType>::error(p,c);
  }

  static inline bool isInfNotSpecificToNearest(const PackArgs&p){
    return p.isOneArgNanInf();
  }

  static inline void check(const PackArgs& p,const RealType & c){
  }

  static inline void twoSum(const RealType& a,const RealType& b, RealType& x,RealType& y ){
    const PackArgs p(a,b);
    x=AddOp<REAL>::nearestOp(p);
    y=AddOp<REAL>::error(p,x);
  }


};


template<typename REAL>
class SubOp{
public:
  typedef REAL RealType;
  typedef vr_packArg<RealType,2> PackArgs;

#ifdef DEBUG_PRINT_OP
  static const char* OpName(){return "sub";}
#endif



  
  static inline RealType nearestOp (const PackArgs&  p) {
    const RealType & a(p.arg1);
    const RealType & b(p.arg2);
    return a-b;
  }

  static inline RealType error (const PackArgs& p, const RealType& x) {
    const RealType & a(p.arg1);
    const RealType & b(-p.arg2);
    const RealType z=x-a;
    return ((a-(x-z)) + (b-z)); //algo TwoSum
  }

  static inline bool isInfNotSpecificToNearest(const PackArgs&p){
    return p.isOneArgNanInf();
  }

  static inline RealType sameSignOfError (const PackArgs& p,const RealType& c) {
    return SubOp<RealType>::error(p,c);
  }

  static inline void check(const PackArgs& p,const RealType & c){
  }

};




//splitFactor used by MulOp
template<class REALTYPE>
REALTYPE splitFactor(){
  return 0./ 0.; //nan to make sure not used
}

template<>
double splitFactor<double>(){
  return 134217729; //((2^27)+1); /27 en double  sup(53/2) /
}

template<>
float splitFactor<float>(){
  return 4097; //((2^12)+1); / 24/2 en float/
}




template<typename REAL>
class MulOp{
public:
  typedef REAL RealType;
  typedef vr_packArg<RealType,2> PackArgs;

#ifdef DEBUG_PRINT_OP
  static const char* OpName(){return "mul";}
#endif


  static inline RealType nearestOp (const PackArgs& p) {
    const RealType & a(p.arg1);
    const RealType & b(p.arg2);
    return a*b;
  };

  static inline RealType error (const PackArgs& p, const RealType& x) {
    /*Provient de "Accurate Sum and dot product" OGITA RUMP OISHI */
    const RealType & a(p.arg1);
    const RealType & b(p.arg2);
    //    return __builtin_fma(a,b,-x);
    //    VG_(umsg)("vr_fma \n");
#ifdef    USE_VERROU_FMA
    RealType c;
    c=vr_fma(a,b,-x);
    return c;
#else
    RealType a1,a2;
    RealType b1,b2;
    MulOp<RealType>::split(a,a1,a2);
    MulOp<RealType>::split(b,b1,b2);

    return (((a1*b1-x)+a1*b2+a2*b1)+a2*b2);
#endif
  };




  static inline void split(RealType a, RealType& x, RealType& y){
    //    const RealType factor=134217729; //((2^27)+1); /*27 en double*/
    const RealType factor(splitFactor<RealType>());
    const RealType c=factor*a;
    x=(c-(c-a));
    y=(a-x);
  }


  static inline RealType sameSignOfError (const PackArgs& p,const RealType& c) {
    if(c!=0){
      return MulOp<RealType>::error(p,c);
    }else{
      if(p.arg1==0 ||p.arg2==0){
	return 0;
      }
      if(p.arg1>0){
	return p.arg2;
      }else{
	return -p.arg2;
      }
    }
  };

  static inline bool isInfNotSpecificToNearest(const PackArgs&p){
    return p.isOneArgNanInf();
  }

  static inline void check(const PackArgs& p,const RealType & c){
  };

  static inline void twoProd(const RealType& a,const RealType& b, RealType& x,RealType& y ){
    const PackArgs p(a,b);
    x=MulOp<REAL>::nearestOp(p);
    y=MulOp<REAL>::error(p,x);
  }

};


template<>
class MulOp<float>{
public:
  typedef float RealType;
  typedef vr_packArg<RealType,2> PackArgs;

#ifdef DEBUG_PRINT_OP
  static const char* OpName(){return "mul";}
#endif

  static inline RealType nearestOp (const PackArgs& p) {
    const RealType & a(p.arg1);
    const RealType & b(p.arg2);
    return a*b;
  };

  static inline RealType error (const PackArgs& p, const RealType& x) {
    /*Provient de "Accurate Sum and dot product" OGITA RUMP OISHI */
    const RealType a(p.arg1);
    const RealType b(p.arg2);
    //    return __builtin_fma(a,b,-x);
    //    VG_(umsg)("vr_fma \n");
#ifdef    USE_VERROU_FMA
    RealType c;
    c=vr_fma(a,b,-x);
    return c;
#else
    RealType a1,a2;
    RealType b1,b2;
    MulOp<RealType>::split(a,a1,a2);
    MulOp<RealType>::split(b,b1,b2);

    return (((a1*b1-x)+a1*b2+a2*b1)+a2*b2);
#endif
  };




  static inline void split(RealType a, RealType& x, RealType& y){
    //    const RealType factor=134217729; //((2^27)+1); /*27 en double*/
    const RealType factor(splitFactor<RealType>());
    const RealType c=factor*a;
    x=(c-(c-a));
    y=(a-x);
  }


  static inline RealType sameSignOfError (const PackArgs& p,const RealType& c) {
    double res=MulOp<double>::error(vr_packArg<double,2>((double)p.arg1,(double)p.arg2) ,(double)c);
    if(res<0){
      return -1;
    }
    if(res>0){
      return 1;
    }
    return 0.;
  };

  static inline bool isInfNotSpecificToNearest(const PackArgs&p){
    return p.isOneArgNanInf();
  }

  static inline void check(const PackArgs& p,const RealType & c){
  };

  static inline void twoProd(const RealType& a,const RealType& b, RealType& x,RealType& y ){
    const PackArgs p(a,b);
    x=MulOp<float>::nearestOp(p);
    y=MulOp<float>::error(p,x);
  }

};





template<typename REAL>
class DivOp{
public:
  typedef REAL RealType;
  typedef vr_packArg<RealType,2> PackArgs;

#ifdef DEBUG_PRINT_OP
  static const char* OpName(){return "div";}
#endif

  static RealType inline nearestOp (const PackArgs& p) {
    const RealType & a(p.arg1);
    const RealType & b(p.arg2);
    return a/b;
  };

  static inline RealType error (const PackArgs& p, const RealType& c) {
    const RealType & x(p.arg1);
    const RealType & y(p.arg2);
#ifdef    USE_VERROU_FMA
    const RealType r=-vr_fma(c,y,-x);
    return r/y;
#else
    RealType u,uu;
    MulOp<RealType>::twoProd(c,y,u,uu);
    return ( x-u-uu)/y ;
#endif
  };

  static inline RealType sameSignOfError (const PackArgs& p,const RealType& c) {
    const RealType & x(p.arg1);
    const RealType & y(p.arg2);
#ifdef    USE_VERROU_FMA
    const RealType r=-vr_fma(c,y,-x);
    return r*y;
#else
    RealType u,uu;
    MulOp<RealType>::twoProd(c,y,u,uu);
    return ( x-u-uu)*y ;
#endif
  };


  static inline void check(const PackArgs& p,const RealType & c){
  };

  static inline bool isInfNotSpecificToNearest(const PackArgs&p){
    return (isNanInf<RealType>(p.arg1))||(p.arg2==RealType(0.));
  }
};


template<>
class DivOp<float>{
public:
  typedef float RealType;
  typedef vr_packArg<RealType,2> PackArgs;

#ifdef DEBUG_PRINT_OP
  static const char* OpName(){return "div";}
#endif

  static RealType inline nearestOp (const PackArgs& p) {
    const RealType & a(p.arg1);
    const RealType & b(p.arg2);
    return a/b;
  };

  static inline RealType error (const PackArgs& p, const RealType& c) {
    const RealType & x(p.arg1);
    const RealType & y(p.arg2);
#ifdef    USE_VERROU_FMA
    const RealType r=-vr_fma(c,y,-x);
    return r/y;
#else
    RealType u,uu;
    MulOp<RealType>::twoProd(c,y,u,uu);
    return ( x-u-uu)/y ;
#endif
  };

  static inline RealType sameSignOfError (const PackArgs& p,const RealType& c) {
    const double x((double)p.arg1);
    const double y((double) p.arg2);
#ifdef    USE_VERROU_FMA
    const double r=-vr_fma((double)c,y,-x);

    if(r>0){return p.arg2;}
    if(r<0){return -p.arg2;}
    //if(r==0){
      return 0.;
      //}
#else
    RealType u,uu;
    MulOp<RealType>::twoProd(c,y,u,uu);
    return ( x-u-uu)*y ;
#endif
  };


  static inline void check(const PackArgs& p,const RealType & c){
  };

  static inline bool isInfNotSpecificToNearest(const PackArgs&p){
    return (isNanInf<RealType>(p.arg1))||(p.arg2==RealType(0.));
  }

};



template<typename REAL>
class MAddOp{
public:
  typedef REAL RealType;
  typedef vr_packArg<RealType,3> PackArgs;

#ifdef DEBUG_PRINT_OP
  static const char* OpName(){return "madd";}
#endif

  
  static RealType inline nearestOp (const PackArgs& p) {
#ifdef    USE_VERROU_FMA
    const RealType & a(p.arg1);
    const RealType & b(p.arg2);
    const RealType & c(p.arg3);
    return vr_fma(a,b,c);
#else
    return 0./0.;
#endif
  };

  static inline RealType error (const PackArgs& p, const RealType& z) {
    //ErrFmaApp : Exact and Aproximated Error of the FMA By Boldo and Muller
    const RealType & a(p.arg1);
    const RealType & x(p.arg2);
    const RealType & b(p.arg3);

    RealType ph,pl;
    MulOp<RealType>::twoProd(a,x, ph,pl);

    RealType uh,ul;
    AddOp<RealType>::twoSum(b,ph, uh,ul);

    const RealType t(uh-z);
    return (t+(pl+ul)) ;
  };

  static inline RealType sameSignOfError (const PackArgs& p,const RealType& c) {
    return error(p,c) ;
  };


  static inline void check(const PackArgs& p, const RealType& d){
  };

  static inline bool isInfNotSpecificToNearest(const PackArgs&p){
    return p.isOneArgNanInf();
  }

};



template<typename REALINPUT, typename REALOUTPUT>
class CastOp{
public:
  typedef REALINPUT RealTypeIn;
  typedef REALOUTPUT RealTypeOut;
  typedef RealTypeOut RealType;
  typedef vr_packArg<RealTypeIn,1> PackArgs;
  

#ifdef DEBUG_PRINT_OP
  static const char* OpName(){return "cast";}
#endif


  
  static inline RealTypeOut nearestOp (const PackArgs& p) {
    const RealTypeIn & in(p.arg1);
    return (RealTypeOut)in;
  };

  static inline RealTypeOut error (const PackArgs& p, const RealTypeOut& z) {
    const RealTypeIn & a(p.arg1);
    const RealTypeIn errorHo= a- (RealTypeIn)z;
    return (RealTypeOut) errorHo;
  };

  static inline RealTypeOut sameSignOfError (const PackArgs& p,const RealTypeOut& c) {
    return error(p,c) ;
  };

  static inline bool isInfNotSpecificToNearest(const PackArgs&p){
    return p.isOneArgNanInf();
  }

  static inline void check(const PackArgs& p, const RealTypeOut& d){
  };

};
