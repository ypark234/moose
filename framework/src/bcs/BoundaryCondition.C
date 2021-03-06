//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "BoundaryCondition.h"
#include "Problem.h"
#include "SystemBase.h"
#include "MooseVariableFE.h"

template <>
InputParameters
validParams<BoundaryCondition>()
{
  return BoundaryCondition::validParams();
}

InputParameters
BoundaryCondition::validParams()
{
  InputParameters params = MooseObject::validParams();
  params += TransientInterface::validParams();
  params += BoundaryRestrictableRequired::validParams();
  params += TaggingInterface::validParams();

  params.addRequiredParam<NonlinearVariableName>(
      "variable", "The name of the variable that this boundary condition applies to");
  params.addParam<bool>("use_displaced_mesh",
                        false,
                        "Whether or not this object should use the "
                        "displaced mesh for computation.  Note that "
                        "in the case this is true but no "
                        "displacements are provided in the Mesh block "
                        "the undisplaced mesh will still be used.");

  params.addParamNamesToGroup("use_displaced_mesh", "Advanced");
  params.addCoupledVar("displacements", "The displacements");
  params.declareControllable("enable");
  params.registerBase("BoundaryCondition");

  return params;
}

BoundaryCondition::BoundaryCondition(const InputParameters & parameters, bool nodal)
  : MooseObject(parameters),
    BoundaryRestrictableRequired(this, nodal),
    SetupInterface(this),
    FunctionInterface(this),
    DistributionInterface(this),
    UserObjectInterface(this),
    TransientInterface(this),
    PostprocessorInterface(this),
    VectorPostprocessorInterface(this),
    GeometricSearchInterface(this),
    Restartable(this, "BCs"),
    MeshChangedInterface(parameters),
    TaggingInterface(this),
    _subproblem(*getCheckedPointerParam<SubProblem *>("_subproblem")),
    _fe_problem(*getCheckedPointerParam<FEProblemBase *>("_fe_problem_base")),
    _sys(*getCheckedPointerParam<SystemBase *>("_sys")),
    _tid(parameters.get<THREAD_ID>("_tid")),
    _assembly(_subproblem.assembly(_tid)),
    _mesh(_subproblem.mesh())
{
}
