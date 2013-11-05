// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2008-2012 by Andreas Lauser                               *
 *   Copyright (C) 2010 by Philipp Nuske                                     *
 *   Copyright (C) 2010 by Bernd Flemisch                                    *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 * \copydoc Opm::VanGenuchten
 */
#ifndef OPM_VAN_GENUCHTEN_HH
#define OPM_VAN_GENUCHTEN_HH

#include "VanGenuchtenParams.hpp"

#include <algorithm>
#include <cmath>
#include <cassert>

namespace Opm {
/*!
 * \ingroup FluidMatrixInteractions
 *
 * \brief Implementation of the van Genuchten capillary pressure -
 *        saturation relation.
 *
 * This class only implements the "raw" van-Genuchten curves as static
 * members and doesn't concern itself converting absolute to effective
 * saturations and vice versa.
 *
 * The converion from and to effective saturations can be done using,
 * e.g. EffToAbsLaw.
 *
 * \see VanGenuchtenParams
 */
template <class ScalarT,
          int wPhaseIdxV,
          int nPhaseIdxV,
          class ParamsT = VanGenuchtenParams<ScalarT> >
class VanGenuchten
{
public:
    //! The type of the parameter objects for this law
    typedef ParamsT Params;

    //! The type of the scalar values for this law
    typedef typename Params::Scalar Scalar;

    //! The number of fluid phases to which this capillary pressure law applies
    enum { numPhases = 2 };

    //! The index of the wetting phase
    enum { wPhaseIdx = wPhaseIdxV };

    //! The index of the non-wetting phase
    enum { nPhaseIdx = nPhaseIdxV };

    /*!
     * \brief The capillary pressure-saturation curves according to van Genuchten.
     *
     * Van Genuchten's empirical capillary pressure <-> saturation
     * function is given by
     * \f[
     * p_{c,wn} = p_n - p_w = ({S_w}^{-1/m} - 1)^{1/n}/\alpha
     * \f]
     *
     * \param values A random access container which stores the
     *               relative pressure of each fluid phase.
     * \param params The parameter object expressing the coefficients
     *               required by the van Genuchten law.
     * \param fs The fluid state for which the capillary pressure
     *           ought to be calculated
     */
    template <class Container, class FluidState>
    static void capillaryPressures(Container &values, const Params &params, const FluidState &fs)
    {
        values[wPhaseIdx] = 0.0; // reference phase
        values[nPhaseIdx] = pcwn(params, fs);
    }

    /*!
     * \brief The relative permeability-saturation curves according to van Genuchten.
     *
     * \param values A random access container which stores the
     *               relative permeability of each fluid phase.
     * \param params The parameter object expressing the coefficients
     *               required by the van Genuchten law.
     * \param fs The fluid state for which the relative permeabilities
     *           ought to be calculated
     */
    template <class Container, class FluidState>
    static void relativePermeabilities(Container &values, const Params &params, const FluidState &fs)
    {
        values[wPhaseIdx] = krw(params, fs);
        values[nPhaseIdx] = krn(params, fs);
    }

    /*!
     * \brief The capillary pressure-saturation curve according to van Genuchten.
     *
     * Van Genuchten's empirical capillary pressure <-> saturation
     * function is given by
     * \f[
     * p_{c,wn} = p_n - p_w = ({S_w}^{-1/m} - 1)^{1/n}/\alpha
     * \f]
     *
     * \param params The parameter object expressing the coefficients
     *               required by the van Genuchten law.
     * \param fs The fluid state for which the capillary pressure
     *           ought to be calculated
     */
    template <class FluidState>
    static Scalar pcwn(const Params &params, const FluidState &fs)
    {
        Scalar Sw = fs.saturation(wPhaseIdx);
        assert(0 <= Sw && Sw <= 1);
        return std::pow(std::pow(Sw, -1.0/params.vgM()) - 1, 1.0/params.vgN())/params.vgAlpha();
    }

    /*!
     * \brief The saturation-capillary pressure curve according to van Genuchten.
     *
     * This is the inverse of the capillary pressure-saturation curve:
     * \f[
     * S_w = {p_C}^{-1} = ((\alpha p_C)^n + 1)^{-m}
     * \f]
     *
     * \param params The parameter object expressing the coefficients
     *               required by the van Genuchten law.
     * \param fs The fluid state containing valid phase pressures
     */
    template <class FluidState>
    static Scalar Sw(const Params &params, const FluidState &fs)
    {
        Scalar pC = fs.pressure(nPhaseIdx) - fs.pressure(wPhaseIdx);
        assert(pC >= 0);

        return std::pow(std::pow(params.vgAlpha()*pC, params.vgN()) + 1, -params.vgM());
    }

    /*!
     * \brief The partial derivative of the capillary pressure with
     *        regard to the saturation according to van Genuchten.
     *
     * This is equivalent to
     * \f[
     * \frac{\partial p_C}{\partial S_w} =
     * -\frac{1}{\alpha n} ({S_w}^{-1/m} - 1)^{1/n - 1} {S_w}^{-1/m - 1}
     * \f]
     *
     * \param params The parameter object expressing the coefficients
     *               required by the van Genuchten law.
     * \param fs The fluid state containing valid saturations
     */
    template <class FluidState>
    static Scalar dpcwn_dSw(const Params &params, const FluidState &fs)
    {
        Scalar Sw = fs.saturation(wPhaseIdx);

        assert(0 < Sw && Sw < 1);

        Scalar powSw = std::pow(Sw, -1/params.vgM());
        return
            - 1/(params.vgAlpha() * params.vgN() * Sw)
            * std::pow(powSw - 1, 1/params.vgN() - 1) * powSw;
    }

    /*!
     * \brief The relative permeability for the wetting phase of the
     *        medium according to van Genuchten's curve with Mualem
     *        parameterization.
     *
     * \param params The parameter object expressing the coefficients
     *               required by the van Genuchten law.
     * \param fs The fluid state for which the relative permeability
     *           ought to be calculated
     */
    template <class FluidState>
    static Scalar krw(const Params &params, const FluidState &fs)
    {
        Scalar Sw = fs.saturation(wPhaseIdx);

        assert(0 <= Sw && Sw <= 1);

        Scalar r = 1. - std::pow(1 - std::pow(Sw, 1/params.vgM()), params.vgM());
        return std::sqrt(Sw)*r*r;
    }

    /*!
     * \brief The derivative of the relative permeability of the
     *        wetting phase in regard to the wetting saturation of the
     *        medium implied according to the van Genuchten curve with
     *        Mualem parameters.
     *
     * \param params The parameter object expressing the coefficients
     *               required by the van Genuchten law.
     * \param fs The fluid state for which the derivative
     *           ought to be calculated
     */
    template <class FluidState>
    static Scalar dkrw_dSw(const Params &params, const FluidState &fs)
    {
        Scalar Sw = fs.saturation(wPhaseIdx);

        assert(0 <= Sw && Sw <= 1);

        const Scalar x = 1 - std::pow(Sw, 1.0/params.vgM());
        const Scalar xToM = std::pow(x, params.vgM());
        return (1 - xToM)/std::sqrt(Sw) * ( (1 - xToM)/2 + 2*xToM*(1-x)/x );
    }


    /*!
     * \brief The relative permeability for the non-wetting phase
     *        of the medium according to van Genuchten.
     *
     * \param params The parameter object expressing the coefficients
     *               required by the van Genuchten law.
     * \param fs The fluid state for which the derivative
     *           ought to be calculated
     */
    static Scalar krn(const Params &params, const FluidState &fs)
    {
        Scalar Sw = fs.saturation(wPhaseIdx);

        assert(0 <= Sw && Sw <= 1);

        return
            std::pow(1 - Sw, 1.0/3) *
            std::pow(1 - std::pow(Sw, 1/params.vgM()), 2*params.vgM());
    }

    /*!
     * \brief The derivative of the relative permeability for the
     *        non-wetting phase in regard to the wetting saturation of
     *        the medium as implied by the van Genuchten
     *        parameterization.
     *
     * \param params The parameter object expressing the coefficients
     *               required by the van Genuchten law.
     * \param fs The fluid state for which the derivative
     *           ought to be calculated
     */
    template <class FluidState>
    static Scalar dkrn_dSw(const Params &params, const FluidState &fs)
    {
        Scalar fs = fs.saturation(wPhaseIdx);

        assert(0 <= Sw && Sw <= 1);

        const Scalar x = std::pow(Sw, 1.0/params.vgM());
        return
            -std::pow(1 - x, 2*params.vgM())
            *std::pow(1 - Sw, -2/3)
            *(1.0/3 + 2*x/Sw);
    }
};
} // namespace Opm

#endif
