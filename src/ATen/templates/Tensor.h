#pragma once

#include "ATen/Scalar.h"
#include "ATen/Type.h"
#include "ATen/TensorImpl.h"
#include "ATen/Utils.h"
#include "ATen/TensorAccessor.h"

namespace at {
class Type;

struct Tensor {

  Tensor()
  : pImpl(nullptr){}
  explicit Tensor(TensorImpl * self, bool retain = true)
  : pImpl(self) {
    if(pImpl != nullptr && retain)
      pImpl->retain();
  }
  Tensor(Tensor const & rhs)
  : pImpl(rhs.pImpl) {
    if(pImpl != nullptr)
      pImpl->retain();
  }
  Tensor(Tensor && rhs)
  : pImpl(rhs.pImpl) {
    rhs.pImpl = nullptr;
  }
  ~Tensor() {
    if(pImpl != nullptr)
      pImpl->release();
  }
  Tensor & operator=(Tensor && rhs) {
    rhs.swap(*this);
    return *this;
  }
  Tensor & operator=(Tensor const & rhs) {
      //Tensor ctor retains original rhs.pImpl
      //then rhs.pImpl is swapped with this->pImpl
      //finally Tensor dtor releases rhs.pImpl, which was originally this->pImpl
      Tensor(rhs).swap(*this);
      return *this;
  }
  void reset() {
    Tensor().swap(*this);
  }
  void reset(TensorImpl * rhs) {
    Tensor(rhs).swap(*this);
  }
  void reset(TensorImpl * rhs, bool retain) {
    Tensor(rhs, retain).swap(*this );
  }
  TensorImpl * get() {
    return pImpl;
  }
  TensorImpl * detach() {
    TensorImpl * ret = pImpl;
    pImpl = nullptr;
    return ret;
  }
  bool defined() const {
    return pImpl != nullptr;
  }
  void swap(Tensor & rhs) {
    TensorImpl * tmp = pImpl;
    pImpl = rhs.pImpl;
    rhs.pImpl = tmp;
  }
  const char * toString() const {
    return pImpl->toString();
  }
  IntList sizes() const {
    return pImpl->sizes();
  }
  IntList strides() const {
    return pImpl->strides();
  }
  Type & type() const {
    return pImpl->type();
  }
  Tensor toType(Type & t) const {
    if(type().ID() ==t.ID())
      return *this;
    return t.copy(*this);
  }
  Tensor & copy_(const Tensor & src) {
    resize_(src.sizes());
    type().copy(src,*this);
    return *this;
  }
  Tensor toType(ScalarType t) {
    return toType(type().toScalarType(t));
  }
  Tensor toBackend(Backend b) {
    return toType(type().toBackend(b));
  }
  int64_t dim() const {
    return ndimension();
  }
  template<typename T>
  T * data() const;

  //toLongData(), toFloatData() etc.
  #define TO_TYPE_DATA(T,name,_) \
  T * to##name##Data() const;
  AT_FORALL_SCALAR_TYPES(TO_TYPE_DATA)
  #undef TO_TYPE_DATA

  template<typename T, size_t N>
  TensorAccessor<T,N> accessor() {
    static_assert(N > 0, "accessor is used for indexing tensor, for scalars use *data<T>()");
    AT_ASSERT(dim() == N, "expected %d dims but tensor has %d",N,dim());
    return TensorAccessor<T,N>(data<T>(),sizes().data(),strides().data());
  }

  Tensor operator-() {
    return neg();
  }
  Tensor& operator+=(const Tensor & other) {
    add_(other);
  }
  Tensor& operator+=(Scalar other) {
    add_(other);
  }
  Tensor& operator-=(const Tensor & other) {
    sub_(other);
  }
  Tensor& operator-=(Scalar other) {
    sub_(other);
  }
  Tensor& operator*=(const Tensor & other) {
    mul_(other);
  }
  Tensor& operator*=(Scalar other) {
    mul_(other);
  }
  Tensor& operator/=(const Tensor & other) {
    div_(other);
  }
  Tensor& operator/=(Scalar other) {
    div_(other);
  }
  Tensor operator[](int64_t idx) {
    return select(0,idx);
  }

  //example
  //Tensor * add(Tensor & b);
  ${tensor_method_declarations}

  friend class Type;

//TODO(zach): sort out friend classes
public:
  TensorImpl * pImpl;
};

// all static inline to allow for inlining of the non-dynamic part of dispatch
${tensor_method_definitions}

template<typename T>
inline T* Tensor::data() const {
  runtime_error("data() cast to unexpected type.");
}
#define DEFINE_CAST(T,name,_) \
template<> \
inline T* Tensor::data() const { \
  AT_ASSERT(type().scalarType() == ScalarType::name, \
    "expected scalar type % s but found %s", #name, \
    at::toString(type().scalarType())); \
  return static_cast<T*>(this->data_ptr()); \
} \
inline T* Tensor::to##name##Data() const { return data<T>(); }

AT_FORALL_SCALAR_TYPES(DEFINE_CAST)
#undef DEFINE_CAST

} //namespace at
