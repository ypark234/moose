//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "ConstraintWarehouse.h"

// MOOSE includes
#include "ElemElemConstraint.h"
#include "MortarConstraintBase.h"
#include "MooseVariable.h"
#include "NodalConstraint.h"
#include "NodeFaceConstraint.h"
#include "NodeElemConstraint.h"

ConstraintWarehouse::ConstraintWarehouse() : MooseObjectWarehouse<Constraint>(/*threaded=*/false) {}

void
ConstraintWarehouse::addObject(std::shared_ptr<Constraint> object,
                               THREAD_ID /*tid = 0*/,
                               bool /*recurse = true*/)
{
  // Adds to the storage of _all_objects
  MooseObjectWarehouse<Constraint>::addObject(object);

  // Cast the the possible Contraint types
  std::shared_ptr<NodeFaceConstraint> nfc = std::dynamic_pointer_cast<NodeFaceConstraint>(object);
  std::shared_ptr<MortarConstraintBase> mc =
      std::dynamic_pointer_cast<MortarConstraintBase>(object);
  std::shared_ptr<NodalConstraint> nc = std::dynamic_pointer_cast<NodalConstraint>(object);
  std::shared_ptr<ElemElemConstraint> ec = std::dynamic_pointer_cast<ElemElemConstraint>(object);
  std::shared_ptr<NodeElemConstraint> nec = std::dynamic_pointer_cast<NodeElemConstraint>(object);

  // NodeFaceConstraint
  if (nfc)
  {
    MooseMesh & mesh = nfc->getParamTempl<FEProblemBase *>("_fe_problem_base")->mesh();
    unsigned int slave = mesh.getBoundaryID(nfc->getParamTempl<BoundaryName>("slave"));
    bool displaced = nfc->parameters().have_parameter<bool>("use_displaced_mesh") &&
                     nfc->getParamTempl<bool>("use_displaced_mesh");

    if (displaced)
      _displaced_node_face_constraints[slave].addObject(nfc);
    else
      _node_face_constraints[slave].addObject(nfc);
  }

  // MortarConstraint
  else if (mc)
  {
    MooseMesh & mesh = mc->getParamTempl<FEProblemBase *>("_fe_problem_base")->mesh();
    bool displaced = mc->getParamTempl<bool>("use_displaced_mesh");

    auto slave_boundary_id = mesh.getBoundaryID(mc->getParamTempl<BoundaryName>("slave_boundary"));
    auto master_boundary_id =
        mesh.getBoundaryID(mc->getParamTempl<BoundaryName>("master_boundary"));
    auto key = std::make_pair(master_boundary_id, slave_boundary_id);

    if (displaced)
      _displaced_mortar_constraints[key].addObject(mc);
    else
      _mortar_constraints[key].addObject(mc);
  }

  // ElemElemConstraint
  else if (ec)
  {
    bool displaced = ec->parameters().have_parameter<bool>("use_displaced_mesh") &&
                     ec->getParamTempl<bool>("use_displaced_mesh");
    const InterfaceID interface_id = ec->getInterfaceID();

    if (displaced)
      _displaced_element_constraints[interface_id].addObject(ec);
    else
      _element_constraints[interface_id].addObject(ec);
  }

  // NodeElemConstraint
  else if (nec)
  {
    MooseMesh & mesh = nec->getParamTempl<FEProblemBase *>("_fe_problem_base")->mesh();
    SubdomainID slave = mesh.getSubdomainID(nec->getParamTempl<SubdomainName>("slave"));
    SubdomainID master = mesh.getSubdomainID(nec->getParamTempl<SubdomainName>("master"));
    bool displaced = nec->parameters().have_parameter<bool>("use_displaced_mesh") &&
                     nec->getParamTempl<bool>("use_displaced_mesh");

    if (displaced)
      _displaced_node_elem_constraints[std::make_pair(slave, master)].addObject(nec);
    else
      _node_elem_constraints[std::make_pair(slave, master)].addObject(nec);
  }

  // NodalConstraint
  else if (nc)
    _nodal_constraints.addObject(nc);

  else
    mooseError("Unknown type of Constraint object");
}

const std::vector<std::shared_ptr<NodalConstraint>> &
ConstraintWarehouse::getActiveNodalConstraints() const
{
  return _nodal_constraints.getActiveObjects();
}

const std::vector<std::shared_ptr<NodeFaceConstraint>> &
ConstraintWarehouse::getActiveNodeFaceConstraints(BoundaryID boundary_id, bool displaced) const
{
  std::map<BoundaryID, MooseObjectWarehouse<NodeFaceConstraint>>::const_iterator it, end_it;

  if (displaced)
  {
    it = _displaced_node_face_constraints.find(boundary_id);
    end_it = _displaced_node_face_constraints.end();
  }

  else
  {
    it = _node_face_constraints.find(boundary_id);
    end_it = _node_face_constraints.end();
  }

  mooseAssert(it != end_it,
              "Unable to locate storage for NodeFaceConstraint objects for the given boundary id: "
                  << boundary_id);
  return it->second.getActiveObjects();
}

const std::vector<std::shared_ptr<MortarConstraintBase>> &
ConstraintWarehouse::getActiveMortarConstraints(
    const std::pair<BoundaryID, BoundaryID> & mortar_interface_key, bool displaced) const
{
  std::map<std::pair<BoundaryID, BoundaryID>,
           MooseObjectWarehouse<MortarConstraintBase>>::const_iterator it,
      end_it;

  if (displaced)
  {
    it = _displaced_mortar_constraints.find(mortar_interface_key);
    end_it = _displaced_mortar_constraints.end();
  }
  else
  {
    it = _mortar_constraints.find(mortar_interface_key);
    end_it = _mortar_constraints.end();
  }

  mooseAssert(it != end_it,
              "No MortarConstraints exist for the specified master-slave boundary pair, master "
                  << mortar_interface_key.first << " and slave " << mortar_interface_key.second);

  return it->second.getActiveObjects();
}

const std::vector<std::shared_ptr<ElemElemConstraint>> &
ConstraintWarehouse::getActiveElemElemConstraints(const InterfaceID interface_id,
                                                  bool displaced) const
{
  std::map<unsigned int, MooseObjectWarehouse<ElemElemConstraint>>::const_iterator it, end_it;

  if (displaced)
  {
    it = _displaced_element_constraints.find(interface_id);
    end_it = _displaced_element_constraints.end();
  }

  else
  {
    it = _element_constraints.find(interface_id);
    end_it = _element_constraints.end();
  }

  mooseAssert(it != end_it,
              "Unable to locate storage for ElemElemConstraint objects for the given interface id: "
                  << interface_id);
  return it->second.getActiveObjects();
}

const std::vector<std::shared_ptr<NodeElemConstraint>> &
ConstraintWarehouse::getActiveNodeElemConstraints(SubdomainID slave_id,
                                                  SubdomainID master_id,
                                                  bool displaced) const
{
  std::map<std::pair<SubdomainID, SubdomainID>,
           MooseObjectWarehouse<NodeElemConstraint>>::const_iterator it,
      end_it;

  if (displaced)
  {
    it = _displaced_node_elem_constraints.find(std::make_pair(slave_id, master_id));
    end_it = _displaced_node_elem_constraints.end();
  }
  else
  {
    it = _node_elem_constraints.find(std::make_pair(slave_id, master_id));
    end_it = _node_elem_constraints.end();
  }

  mooseAssert(it != end_it,
              "Unable to locate storage for NodeElemConstraint objects for the given slave and "
              "master id pair: ["
                  << slave_id << ", " << master_id << "]");
  return it->second.getActiveObjects();
}

bool
ConstraintWarehouse::hasActiveNodalConstraints() const
{
  return _nodal_constraints.hasActiveObjects();
}

bool
ConstraintWarehouse::hasActiveElemElemConstraints(const InterfaceID interface_id,
                                                  bool displaced) const
{
  std::map<unsigned int, MooseObjectWarehouse<ElemElemConstraint>>::const_iterator it, end_it;

  if (displaced)
  {
    it = _displaced_element_constraints.find(interface_id);
    end_it = _displaced_element_constraints.end();
  }

  else
  {
    it = _element_constraints.find(interface_id);
    end_it = _element_constraints.end();
  }

  return (it != end_it && it->second.hasActiveObjects());
}

bool
ConstraintWarehouse::hasActiveNodeFaceConstraints(BoundaryID boundary_id, bool displaced) const
{
  std::map<BoundaryID, MooseObjectWarehouse<NodeFaceConstraint>>::const_iterator it, end_it;

  if (displaced)
  {
    it = _displaced_node_face_constraints.find(boundary_id);
    end_it = _displaced_node_face_constraints.end();
  }

  else
  {
    it = _node_face_constraints.find(boundary_id);
    end_it = _node_face_constraints.end();
  }

  return (it != end_it && it->second.hasActiveObjects());
}

bool
ConstraintWarehouse::hasActiveNodeElemConstraints(SubdomainID slave_id,
                                                  SubdomainID master_id,
                                                  bool displaced) const
{
  std::map<std::pair<SubdomainID, SubdomainID>,
           MooseObjectWarehouse<NodeElemConstraint>>::const_iterator it,
      end_it;

  if (displaced)
  {
    it = _displaced_node_elem_constraints.find(std::make_pair(slave_id, master_id));
    end_it = _displaced_node_elem_constraints.end();
  }

  else
  {
    it = _node_elem_constraints.find(std::make_pair(slave_id, master_id));
    end_it = _node_elem_constraints.end();
  }

  return (it != end_it && it->second.hasActiveObjects());
}

void ConstraintWarehouse::updateActive(THREAD_ID /*tid*/)
{
  MooseObjectWarehouse<Constraint>::updateActive();
  _nodal_constraints.updateActive();

  for (auto & it : _node_face_constraints)
    it.second.updateActive();

  for (auto & it : _displaced_node_face_constraints)
    it.second.updateActive();

  // FIXME: We call updateActive() on the NodeFaceConstraints again?
  for (auto & it : _node_face_constraints)
    it.second.updateActive();

  for (auto & it : _element_constraints)
    it.second.updateActive();

  for (auto & it : _node_elem_constraints)
    it.second.updateActive();
}

void
ConstraintWarehouse::subdomainsCovered(std::set<SubdomainID> & subdomains_covered,
                                       std::set<std::string> & unique_variables,
                                       THREAD_ID /*tid=0*/) const
{
  // Loop over undisplaced
  for (const auto & pr : _mortar_constraints)
  {
    const auto & objects = pr.second.getActiveObjects();
    for (const auto & mc : objects)
    {
      const MooseVariableFEBase * lm_var = mc->variable();
      if (lm_var)
      {
        unique_variables.insert(lm_var->name());
        const std::set<SubdomainID> & lm_subdomains = lm_var->activeSubdomains();
        subdomains_covered.insert(lm_subdomains.begin(), lm_subdomains.end());
      }

      // Mortar constraints require the creation of a master lower dimensional subdomain in order to
      // create the mortar segment mesh. We don't need any computing objects on it
      subdomains_covered.insert(mc->masterSubdomain());
    }
  }

  // Loop over displaced
  for (const auto & pr : _displaced_mortar_constraints)
  {
    const auto & objects = pr.second.getActiveObjects();
    for (const auto & mc : objects)
    {
      const MooseVariableFEBase * lm_var = mc->variable();
      if (lm_var)
      {
        unique_variables.insert(lm_var->name());
        const std::set<SubdomainID> & lm_subdomains = lm_var->activeSubdomains();
        subdomains_covered.insert(lm_subdomains.begin(), lm_subdomains.end());
      }

      // Mortar constraints require the creation of a master lower dimensional subdomain in order to
      // create the mortar segment mesh. We don't need any computing objects on it
      subdomains_covered.insert(mc->masterSubdomain());
    }
  }
}

void
ConstraintWarehouse::residualEnd(THREAD_ID tid /* = 0*/) const
{
  checkThreadID(tid);
  for (const auto & object : _active_objects[tid])
    object->residualEnd();
}
