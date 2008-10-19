/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    itkDiscreteHessianGaussianImageFunction.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef __itkDiscreteHessianGaussianImageFunction_h
#define __itkDiscreteHessianGaussianImageFunction_h

#include "itkNeighborhoodOperatorImageFunction.h"
#include "itkImageFunction.h"
#include "itkGaussianOperator.h"
#include "itkGaussianDerivativeOperator.h"
#include "itkSymmetricSecondRankTensor.h"

namespace itk
{

/**
 * \class DiscreteHessianGaussianImageFunction
 * \brief Compute the Hessian Gaussian of an image at a specific location in space
          by calculating discrete second-order gaussian derivatives.
 * This class is templated over the input image type.
 *
 * \author Iv�n Mac�a, VICOMTech, Spain, http://www.vicomtech.es
 *
 * This implementation was taken from the Insight Journal paper:
 * http://hdl.handle.net/1926/179
 *
 * \sa NeighborhoodOperator
 * \sa ImageFunction
 */
template <class TInputImage,class TOutput=double>
class ITK_EXPORT DiscreteHessianGaussianImageFunction :
  public ImageFunction< TInputImage,
    SymmetricSecondRankTensor<TOutput,::itk::GetImageDimension<TInputImage>::ImageDimension>,
    TOutput >
{
public:

  /**Standard "Self" typedef */
  typedef DiscreteHessianGaussianImageFunction   Self;

  /** Standard "Superclass" typedef*/
  typedef ImageFunction<TInputImage,
    SymmetricSecondRankTensor<TOutput,::itk::GetImageDimension<TInputImage>::ImageDimension>,
    TOutput > Superclass;

  /** Smart pointer typedef support. */
  typedef SmartPointer<Self>        Pointer;
  typedef SmartPointer<const Self>  ConstPointer;

  /** Method for creation through the object factory.*/
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro( DiscreteHessianGaussianImageFunction, ImageFunction );

  /** Image dependent types.*/
  typedef typename Superclass::InputImageType       InputImageType;
  typedef typename Superclass::InputPixelType       InputPixelType;
  typedef typename Superclass::IndexType            IndexType;
  typedef typename Superclass::ContinuousIndexType  ContinuousIndexType;
  typedef typename Superclass::PointType            PointType;

  /** Dimension of the underlying image. */
  itkStaticConstMacro(ImageDimension2, unsigned int,
                      InputImageType::ImageDimension);

  /** Output type. */
  typedef SymmetricSecondRankTensor<TOutput,
    ::itk::GetImageDimension<TInputImage>::ImageDimension>  TensorType;
  typedef typename Superclass::OutputType                   OutputType;

  typedef FixedArray<double,itkGetStaticConstMacro(ImageDimension2)>  VarianceArrayType;

  typedef itk::GaussianDerivativeOperator<TOutput,
    itkGetStaticConstMacro(ImageDimension2)>            GaussianDerivativeOperatorType;

  /** Array to store gaussian derivative operators from zero to second order
    * (3*ImageDimension operators). */
  typedef FixedArray<GaussianDerivativeOperatorType,
    3*itkGetStaticConstMacro(ImageDimension2)>          GaussianDerivativeOperatorArrayType;

  typedef Neighborhood<TOutput,itkGetStaticConstMacro(ImageDimension2)>  KernelType;

  /** Array to store precomputed N-dimensional kernels for the hessian components. */
  typedef FixedArray<KernelType,itkGetStaticConstMacro(ImageDimension2)*
    (itkGetStaticConstMacro(ImageDimension2)+1)/2>      KernelArrayType;

  /** Image function that performs convolution with the neighborhood operator. */
  typedef NeighborhoodOperatorImageFunction
    <InputImageType, TOutput>                           OperatorImageFunctionType;
  typedef typename OperatorImageFunctionType::Pointer   OperatorImageFunctionPointer;

  /** Interpolation modes. */
  enum InterpolationModeType { NearestNeighbourInterpolation, LinearInterpolation };

public:

  /** Evalutate the  in the given dimension at specified point */
  virtual OutputType Evaluate(const PointType& point) const;

  /** Evaluate the function at specified Index position*/
  virtual OutputType EvaluateAtIndex( const IndexType & index ) const;

  /** Evaluate the function at specified ContinousIndex position.*/
  virtual OutputType EvaluateAtContinuousIndex(
    const ContinuousIndexType & index ) const;

  /** The variance for the discrete Gaussian kernel.  Sets the variance
   * independently for each dimension, but see also SetVariance(const double v).
   * The default is 0.0 in each dimension. If UseImageSpacing is true,
   * the units are the physical units of your image.  If UseImageSpacing
   * is false then the units are pixels.*/
  void SetVariance( const double* variance );
  void SetVariance( const double variance );
  const double* GetVariance() const { return m_Variance; }

  /** Convenience method for setting the variance through the standard deviation. */
  void SetSigma( const double* sigma );
  void SetSigma( const double sigma );

  /** Sets the desired maximum error of the gaussian approximation.  Maximum
   * error is the difference between the area under the discrete Gaussian curve
   * and the area under the continuous Gaussian. Maximum error affects the
   * Gaussian operator size. The value is clamped between 0.00001 and 0.99999. */
  void SetMaximumError( const double maxerror )
  {
    const double Min = 0.00001;
    const double Max = 1.0 - Min;
    if ( m_MaximumError != (maxerror<Min?Min:(maxerror>Max?Max:maxerror)) )
      {
      m_MaximumError = (maxerror<Min?Min:(maxerror>Max?Max:maxerror));
      this->RecomputeGaussianKernel();
      this->Modified();
      }
  }

  /** Set/Get the flag for calculating scale-space normalized derivatives.
    * Normalized derivatives are obtained multiplying by the scale parameter t. */
  void SetNormalizeAcrossScale( bool flag )
  {
    if ( m_NormalizeAcrossScale != flag )
      {
        m_NormalizeAcrossScale = flag;
        this->RecomputeGaussianKernel();
        this->Modified();
      }
  }
  bool GetNormalizeAcrossScale() const { return m_NormalizeAcrossScale; }
  itkBooleanMacro(NormalizeAcrossScale);

  /** Set/Get the flag for using image spacing when calculating derivatives. */
  void SetUseImageSpacing( bool flag )
  {
    if ( m_UseImageSpacing != flag )
      {
        m_UseImageSpacing = flag;
        this->RecomputeGaussianKernel();
        this->Modified();
      }
  }
  bool GetUseImageSpacing() const { return m_UseImageSpacing; }
  itkBooleanMacro(UseImageSpacing);

  /** Returns the maximum error of the gaussian approximation.  Maximum error
   * is the difference between the area under the discrete Gaussian curve and
   * the area under the continuous Gaussian. Maximum error affects the Gaussian
   * operator size. */
  double GetMaximumError() { return m_MaximumError; }

  /** Sets a limit for growth of the kernel.  Small maximum error values with
   *  large variances will yield very large kernel sizes.  This value can be
   *  used to truncate a kernel in such instances.  A warning will be given on
   *  truncation of the kernel. */
  void SetMaximumKernelWidth( unsigned int n )
  {
    if ( m_MaximumKernelWidth != n )
      {
      m_MaximumKernelWidth = n;
      this->RecomputeGaussianKernel();
      this->Modified();
      }
  }

  /** Returns the maximum allowed kernel width. */
  unsigned int GetMaximumKernelWidth() const { return m_MaximumKernelWidth; }

  /** Set/Get the interpolation mode. */
  itkSetMacro( InterpolationMode, InterpolationModeType );
  itkGetMacro( InterpolationMode, InterpolationModeType );

   /** Set the input image.
   * \warning this method caches BufferedRegion information.
   * If the BufferedRegion has changed, user must call
   * SetInputImage again to update cached values. */
  virtual void SetInputImage( const InputImageType * ptr );

protected:

  DiscreteHessianGaussianImageFunction();
  DiscreteHessianGaussianImageFunction( const Self& ){};

  ~DiscreteHessianGaussianImageFunction(){};

  void operator=( const Self& ){};
  void PrintSelf(std::ostream& os, Indent indent) const;

  void RecomputeGaussianKernel();
 // void RecomputeContinuousGaussianKernel(
   //        const double* offset) const;

private:

  /** Desired variance of the discrete Gaussian function. */
  VarianceArrayType  m_Variance;

  /** Difference between the areas under the curves of the continuous and
   * discrete Gaussian functions. */
  double     m_MaximumError;

  /** Maximum kernel size allowed.  This value is used to truncate a kernel
   *  that has grown too large.  A warning is given when the specified maximum
   *  error causes the kernel to exceed this size. */
  unsigned int m_MaximumKernelWidth;

  /** Array of derivative operators, one for each dimension and order.
    * First N zero-order operators are stored, then N first-order and
    * then N second-order making 3*N operators altogether where N=ImageDimension. */
  mutable GaussianDerivativeOperatorArrayType   m_OperatorArray;

  /** Array of N-dimensional kernels which are the result of convolving the operators
    * for calculating hessian matrix derivatives. */
  KernelArrayType  m_KernelArray;

  /** OperatorImageFunction */
  OperatorImageFunctionPointer  m_OperatorImageFunction;

  /** Flag for scale-space normalization of derivatives. */
  bool m_NormalizeAcrossScale;

  /** Flag to indicate whether to use image spacing */
  bool m_UseImageSpacing;

  /** Interpolation mode. */
  InterpolationModeType  m_InterpolationMode;

};

} // namespace itk

// Define instantiation macro for this template.
#define ITK_TEMPLATE_DiscreteHessianGaussianImageFunction(_, EXPORT, x, y) namespace itk { \
  _(2(class EXPORT DiscreteHessianGaussianImageFunction< ITK_TEMPLATE_2 x >)) \
  namespace Templates { typedef DiscreteHessianGaussianImageFunction< ITK_TEMPLATE_2 x > \
                                                  DiscreteHessianGaussianImageFunction##y; } \
  }

#if ITK_TEMPLATE_EXPLICIT
# include "Templates/itkDiscreteHessianGaussianImageFunction+-.h"
#endif

#if ITK_TEMPLATE_TXX
# include "itkDiscreteHessianGaussianImageFunction.txx"
#endif

#endif
