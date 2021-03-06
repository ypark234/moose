//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "RANFSTieNode.h"
#include "PenetrationLocator.h"
#include "PenetrationInfo.h"
#include "SystemBase.h"
#include "Assembly.h"
#include "NearestNodeLocator.h"

#include "libmesh/numeric_vector.h"
#include "libmesh/sparse_matrix.h"

registerMooseObject("ContactTestApp", RANFSTieNode);

template <>
InputParameters
validParams<RANFSTieNode>()
{
  InputParameters params = validParams<NodeFaceConstraint>();
  params.set<bool>("use_displaced_mesh") = true;

  MooseEnum component("x=0 y=1 z=2");
  params.addRequiredParam<MooseEnum>(
      "component", component, "The force component constraint that this object is supplying");
  params.addRequiredCoupledVar(
      "displacements",
      "The displacements appropriate for the simulation geometry and coordinate system");
  return params;
}

RANFSTieNode::RANFSTieNode(const InputParameters & parameters)
  : NodeFaceConstraint(parameters),
    _component(getParam<MooseEnum>("component")),
    _mesh_dimension(_mesh.dimension()),
    _residual_copy(_sys.residualGhosted())
{
  // modern parameter scheme for displacements
  for (unsigned int i = 0; i < coupledComponents("displacements"); ++i)
  {
    _vars.push_back(coupled("displacements", i));
    _var_objects.push_back(getVar("displacements", i));
  }

  if (_vars.size() != _mesh_dimension)
    mooseError("The number of displacement variables does not match the mesh dimension!");
}

void
RANFSTieNode::residualSetup()
{
  _node_to_lm.clear();
}

bool
RANFSTieNode::overwriteSlaveResidual()
{
  return _nearest_node;
}

bool
RANFSTieNode::shouldApply()
{
  auto & nearest_node_loc = _penetration_locator._nearest_node;
  _nearest_node = nearest_node_loc.nearestNode(_current_node->id());
  if (_nearest_node)
  {
    auto slave_dof_number = _current_node->dof_number(0, _vars[_component], 0);
    // We overwrite the slave residual so we cannot use the residual
    // copy for determining the Lagrange multiplier when computing the Jacobian
    if (!_subproblem.currentlyComputingJacobian())
      _node_to_lm.insert(std::make_pair(_current_node->id(),
                                        _residual_copy(slave_dof_number) /
                                            _var_objects[_component]->scalingFactor()));
    else
    {
      std::vector<dof_id_type> master_cols;
      std::vector<Number> master_values;

      _jacobian->get_row(slave_dof_number, master_cols, master_values);
      mooseAssert(master_cols.size() == master_values.size(),
                  "The size of the dof container and value container are different");

      _dof_number_to_value.clear();

      for (MooseIndex(master_cols) i = 0; i < master_cols.size(); ++i)
        _dof_number_to_value.insert(
            std::make_pair(master_cols[i], master_values[i] / _var.scalingFactor()));
    }

    mooseAssert(_node_to_lm.find(_current_node->id()) != _node_to_lm.end(),
                "The node " << _current_node->id() << " should map to a lagrange multiplier");
    _lagrange_multiplier = _node_to_lm[_current_node->id()];

    _master_index = _current_master->get_node_index(_nearest_node);
    mooseAssert(_master_index != libMesh::invalid_uint,
                "nearest node not a node on the current master element");

    return true;
  }

  return false;
}

Real
RANFSTieNode::computeQpResidual(Moose::ConstraintType type)
{
  switch (type)
  {
    case Moose::ConstraintType::Slave:
      return (*_current_node - *_nearest_node)(_component);

    case Moose::ConstraintType::Master:
    {
      if (_i == _master_index)
        return _lagrange_multiplier;

      else
        return 0;
    }

    default:
      return 0;
  }
}

Real
RANFSTieNode::computeQpJacobian(Moose::ConstraintJacobianType type)
{
  switch (type)
  {
    case Moose::ConstraintJacobianType::SlaveSlave:
      return _phi_slave[_j][_qp];

    case Moose::ConstraintJacobianType::SlaveMaster:
      if (_master_index == _j)
        return -1;
      else
        return 0;

    case Moose::ConstraintJacobianType::MasterSlave:
      if (_i == _master_index)
      {
        mooseAssert(_dof_number_to_value.find(_connected_dof_indices[_j]) !=
                        _dof_number_to_value.end(),
                    "The connected dof index is not found in the _dof_number_to_value container. "
                    "This must mean that insufficient sparsity was allocated");
        return _dof_number_to_value[_connected_dof_indices[_j]];
      }
      else
        return 0;

    default:
      return 0;
  }
}

void
RANFSTieNode::computeSlaveValue(NumericVector<Number> &)
{
}

Real
RANFSTieNode::computeQpSlaveValue()
{
  mooseError("We overrode commputeSlaveValue so computeQpSlaveValue should never get called");
}
