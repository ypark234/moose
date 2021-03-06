//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "DarcyPressure.h"

registerADMooseObject("DarcyThermoMechApp", DarcyPressure);

template <ComputeStage compute_stage>
InputParameters
DarcyPressure<compute_stage>::validParams()
{
  InputParameters params = ADDiffusion<compute_stage>::validParams();
  params.addClassDescription("Compute the diffusion term for Darcy pressure ($p$) equation: "
                             "$-\\nabla \\cdot \\frac{\\mathbf{K}}{\\mu} \\nabla p = 0$");
  return params;
}

template <ComputeStage compute_stage>
DarcyPressure<compute_stage>::DarcyPressure(const InputParameters & parameters)
  : ADDiffusion<compute_stage>(parameters),
    _permeability(getMaterialProperty<Real>("permeability")),
    _viscosity(getADMaterialProperty<Real>("viscosity"))
{
}

template <ComputeStage compute_stage>
ADRealVectorValue
DarcyPressure<compute_stage>::precomputeQpResidual()
{
  return (_permeability[_qp] / _viscosity[_qp]) *
         ADDiffusion<compute_stage>::precomputeQpResidual();
}
