//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "SinglePhaseFluidProperties.h"

class SimpleFluidProperties;

template <>
InputParameters validParams<SimpleFluidProperties>();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"

/**
 * Fluid properties of a simple, idealised fluid
 * density=density0 * exp(P / bulk_modulus - thermal_expansion * T)
 * internal_energy = cv * T
 * enthalpy = cv * T + P / density
 * The following parameters are constant:
 * thermal expansion
 * cv
 * cp
 * bulk modulus
 * thermal conductivity
 * specific entropy
 * viscosity
 */
class SimpleFluidProperties : public SinglePhaseFluidProperties
{
public:
  SimpleFluidProperties(const InputParameters & parameters);
  virtual ~SimpleFluidProperties();

  virtual std::string fluidName() const override;

  virtual Real molarMass() const override;

  virtual Real beta_from_p_T(Real pressure, Real temperature) const override;

  virtual void beta_from_p_T(Real pressure,
                             Real temperature,
                             Real & beta,
                             Real & dbeta_dp,
                             Real & dbeta_dT) const override;

  virtual Real cp_from_p_T(Real pressure, Real temperature) const override;

  virtual void cp_from_p_T(
      Real pressure, Real temperature, Real & cp, Real & dcp_dp, Real & dcp_dT) const override;

  virtual Real cv_from_p_T(Real pressure, Real temperature) const override;

  virtual Real c_from_p_T(Real pressure, Real temperature) const override;

  virtual Real k_from_p_T(Real pressure, Real temperature) const override;

  virtual void
  k_from_p_T(Real pressure, Real temperature, Real & k, Real & dk_dp, Real & dk_dT) const override;

  virtual Real s_from_p_T(Real pressure, Real temperature) const override;
  virtual void s_from_p_T(Real p, Real T, Real & s, Real & ds_dp, Real & ds_dT) const override;

  virtual Real rho_from_p_T(Real pressure, Real temperature) const override;

  virtual void rho_from_p_T(
      Real pressure, Real temperature, Real & rho, Real & drho_dp, Real & drho_dT) const override;

  virtual Real e_from_p_T(Real pressure, Real temperature) const override;

  virtual void
  e_from_p_T(Real pressure, Real temperature, Real & e, Real & de_dp, Real & de_dT) const override;

  virtual Real mu_from_p_T(Real pressure, Real temperature) const override;

  virtual void mu_from_p_T(
      Real pressure, Real temperature, Real & mu, Real & dmu_dp, Real & dmu_dT) const override;

  virtual Real h_from_p_T(Real p, Real T) const override;

  virtual void
  h_from_p_T(Real pressure, Real temperature, Real & h, Real & dh_dp, Real & dh_dT) const override;

protected:
  /// molar mass
  const Real _molar_mass;

  /// thermal expansion coefficient
  const Real _thermal_expansion;

  /// specific heat at constant volume
  const Real _cv;

  /// specific heat at constant pressure
  const Real _cp;

  /// bulk modulus
  const Real _bulk_modulus;

  /// thermal conductivity
  const Real _thermal_conductivity;

  /// specific entropy
  const Real _specific_entropy;

  /// viscosity
  const Real _viscosity;

  /// density at zero pressure and temperature
  const Real _density0;

  /// Porepressure coefficient: enthalpy = internal_energy + porepressure / density * _pp_coeff
  const Real _pp_coeff;
};

#pragma GCC diagnostic pop
