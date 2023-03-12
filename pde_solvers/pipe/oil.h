#pragma once

/// @brief ����� �����������
struct viscosity_data_point {
    double temperature;
    double kinematic_viscosity;
};


/// @brief ������ �������� (� ����������� �� ����� ������ ��������)
struct oil_viscosity_parameters_t
{
    /// @brief ����������� ����������� ��� ��������
    double nominal_temperature{ KELVIN_OFFSET + 20 }; // 20 �������� �������
    /// @brief �������������� �������� ��� ����������� ����������� ���������
    double nominal_viscosity{ 10e-6 };
    /// @brief ����������� � ������� ��������-����������
    double temperature_coefficient{ 0 };

    /// @brief ������� ����������� ��������-����������
    static double viscosity_Filonov_Reynolds(double default_viscosity,
        double default_temperature, double kinematic_viscosity_temperature_coefficient,
        double temperature)
    {
        const double& k = kinematic_viscosity_temperature_coefficient;
        double viscosity = default_viscosity * exp(-k * (temperature - default_temperature));
        return viscosity;
    }
    /// @brief ������������ ������������� ����������� � ����������� ��� 20 ���� �� ���� ������
    /// (T_1, \nu_1), (T_2, \nu_2)
    static double find_kinematic_viscosity_temperature_coefficient(
        double viscosity1, double temperature1,
        double viscosity2, double temperature2)
    {
        double k = -log(viscosity1 / viscosity2) / (temperature1 - temperature2);
        return k;
    }
    double operator()(double temperature) const
    {
        return viscosity_Filonov_Reynolds(nominal_viscosity, nominal_temperature,
            temperature_coefficient, temperature);
    }
    /// @brief �������������� �������� - ������ ���������� ��� ����������� ������
    double operator()() const
    {
        return nominal_viscosity;
    }

    /// @brief ������������� ������ �������� �� ����������� �� ���� �����
    /// @param viscogramm 
    oil_viscosity_parameters_t(const array<viscosity_data_point, 2>& viscogramm)
    {
        const auto& v = viscogramm;
        temperature_coefficient = find_kinematic_viscosity_temperature_coefficient(
            v[0].kinematic_viscosity, v[0].temperature,
            v[1].kinematic_viscosity, v[1].temperature);

        nominal_temperature = v[0].temperature;
        nominal_viscosity = v[0].kinematic_viscosity;
    }
    /// @brief ��������� �������������
    oil_viscosity_parameters_t() = default;
};

struct oil_density_parameters_t {
    /// @brief ��������� ��� ����������� ��������, ��/�3
    double nominal_density{ 760 };
    /// @brief ������ ��������� ��������, ��
    /// �������� �� ��������� ��������� �� [����� 2017], ���. 77
    double fluid_elasticity_modulus{ 1.5e9 };
    /// @brief ����������� ��������, ��� ������� ������������� ���������, ��
    double nominal_pressure{ ATMOSPHERIC_PRESSURE };

    /// @brief ����������� ����������� ��� �������� (1/��)
    /// �����������, ����������� ��������� ��������� �������� ��� ���������� �������� �� ������������
    /// � ���������� ���������� ��� \beta_\rho
    double getCompressionRatio() const {
        return 1 / fluid_elasticity_modulus;
    }
    /// @brief ��������� ��� ������� ��������, � ������ ������������ ����������� ��������
    double getDensity(double pressure) const {
        return nominal_density * (1 + getCompressionRatio() * (pressure - nominal_pressure));
    }

    /// @brief ��������� ��� ����� ����������� � �������������� ����������
    double operator()() const
    {
        return nominal_density;
    }
};

/// @brief ��������������� �������� ����� (������������� �� �����)
struct oil_heat_parameters_t {
    /// @brief ����������� ���������� �����������, ��*�-2*�-1
    double internalHeatTransferCoefficient = 257;
    /// @brief ������������, ��*��-1*�-1
    double HeatCapacity = 2000;
    /// @brief ����������� ����������, ������� C (���������� �� ��������!)
    double pourPointTemperature{ 12 };
};


/// @brief �������� �����
struct OilParameters
{
    /// @brief ������ ���������
    oil_density_parameters_t density;
    /// @brief ������ ��������
    oil_viscosity_parameters_t viscosity;
    /// @brief �������� ������
    oil_heat_parameters_t heat;



    /// @brief ������������ �� ������� �����
    /// ������� �� ��������� "������ ����� ����������������..."
    double get_heat_capacity_kreg(double temperature) const {
        double Cp = 31.56 / sqrt(density.nominal_density) * (762 + 3.39 * temperature);
        return Cp;
    }

};




/// @brief ������������� ������������� �������� �� �������
struct viscosity_table_model_t {
    static constexpr std::array<double, 3> viscosity_temperatures{
        KELVIN_OFFSET + 0, KELVIN_OFFSET + 20, KELVIN_OFFSET + 50 };

    /// @brief ������������ ������� ������� �������� ��� ������� ��������
    /// @param viscosities ������� ������� ��������
    /// @param viscosity_working ������� ��������
    /// @param viscosity_working_temperature ����������� ��� ������� ���������� �������� ()
    /// @return ������������������ ������� ��������
    static inline array<double, 3> adapt(array<double, 3> viscosities,
        double viscosity_working, double viscosity_working_temperature)
    {
        auto model = reconstruct(viscosities);

        double visc_calc = calc(model, viscosity_working_temperature);
        double mult = viscosity_working / visc_calc;

        for (auto& v : viscosities) {
            v *= mult;
        }

#ifdef _DEBUG
        auto model_adapt = reconstruct(viscosities);
        double visc_adapt = calc(model_adapt, viscosity_working_temperature);
#endif

        return viscosities;
    }


    /// @brief ��������������� ������������� �� ������� ��������
    /// @param viscosities 
    /// @return 
    static inline array<double, 3> reconstruct(const array<double, 3>& viscosities)
    {
        array<double, 3> coeffs{
            std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::quiet_NaN()
        };

        const auto& v = viscosities;

        constexpr double T1 = viscosity_temperatures[0];
        constexpr double T2 = viscosity_temperatures[1];
        constexpr double T3 = viscosity_temperatures[2];

        constexpr double eps1 = 1e-4; // �������� ��������� ��������� �� �� ��������� (!!���� �����������!!)
        constexpr double eps2 = 1e-4; // �������� 

        constexpr double a1 = (T3 - T2) / (T2 - T1);

        double lognu1div2 = log(v[0] / v[1]);
        double nu2div3 = v[1] / v[2];
        double nu3div1 = v[2] / v[0];

        if (abs(nu3div1 - 1) < eps1 && abs(nu2div3 - 1) < eps1) {
            // �������� = const
            coeffs[0] = v[0];
            return coeffs;
        }

        double a = a1 * lognu1div2 / log(nu2div3); // ������, a > 1

        if (abs(a - 1) < eps2) {
            // �������-��������
            coeffs[0] = v[1]; // ��� �������� ��������

            constexpr double DT = T2 - T1;
            coeffs[1] = lognu1div2 / DT; // �������������!
            return coeffs;
        }

        // ������-�������-������
        coeffs[0] = v[2] * pow(nu3div1, 1.0 / (a - 1)); // nu_inf
        coeffs[1] = (T3 - a * T1) / (1 - a); // theta
        const double& theta = coeffs[1];
        coeffs[2] = (T1 - theta) * (T2 - theta) / (T2 - T1) * lognu1div2; // b

        return coeffs;
    }
    /// @brief ������ �������� � ����� �������������
    /// @param coeffs 
    /// @param temperature 
    /// @return 
    static inline double calc(const array<double, 3>& coeffs, double temperature)
    {
        if (!std::isnan(coeffs[2])) {
            // ������-�������-������
            const auto& nu_inf = coeffs[0];
            const auto& theta = coeffs[1];
            const auto& b = coeffs[2];
            return nu_inf * exp(b / (temperature - theta));
        }
        else if (!std::isnan(coeffs[1])) {
            // �������-��������
            const auto& nu_nominal = coeffs[0];
            const auto& k = coeffs[1];
            return nu_nominal * exp(-k * (temperature - viscosity_temperatures[1]));
        }
        else {
            // ���������
            return coeffs[0];
        }
    }
};




/// @brief ������������ (��������������� � �������� �������) ��������� �����
/// @tparam DataBuffer �������� vector<double> ��� ��������, double ��� ��������� ������
template <typename BufferDensity, typename BufferViscosity>
struct fluid_properties_dynamic {
    /// @brief ��������� ��� ����������� (����������, �����������) ��������
    BufferDensity nominal_density;
    /// @brief ������� ��������
    BufferViscosity viscosity_approximation;

    fluid_properties_dynamic(BufferDensity nominal_density, BufferViscosity viscosity_approximation)
        : nominal_density(nominal_density)
        , viscosity_approximation(viscosity_approximation)
    {

    }
};



/// @brief ����������� (���������� � �������� �������) ��������� �����
struct fluid_properties_static {
    // === ��������
    /// @brief �����������, ��� ������� ���������� ��������


    // === ���������
    /// @brief ������ ��������� ��������, ��
    /// �������� �� ��������� ��������� �� [����� 2017], ���. 77
    double fluid_elasticity_modulus{ 1.5e9 };
    /// @brief ����������� ��������, ��� ������� ������������� ���������, ��
    double nominal_pressure{ ATMOSPHERIC_PRESSURE };
    /// @brief ����������� �����������, ��� ������� ������������� ���������, K
    double nominal_temperature{ 20 + KELVIN_OFFSET };


    // === �����������
    /// @brief ������������, ��*��-1*�-1
    double heat_capacity = 2000;
    /// @brief ����������� ����������, ������� K
    double pour_temperature{ 12 + KELVIN_OFFSET };

    /// @brief ����������� ����������� ��� �������� (1/��)
    /// �����������, ����������� ��������� ��������� �������� ��� ���������� �������� �� ������������
    /// � ���������� ���������� ��� \beta_\rho
    double get_compression_ratio() const {
        return 1 / fluid_elasticity_modulus;
    }

};

/// @brief ������� ������� ������
struct fluid_properties_profile_t :
    fluid_properties_dynamic<const vector<double>&, const vector<array<double, 3>>&>,
    fluid_properties_static
{
    // ����� ��� ������� ������� �� ���������� (������� �� �������)

    fluid_properties_profile_t(const vector<double>& nominal_density,
        const vector<array<double, 3>>& viscosity_approximation)
        : fluid_properties_dynamic<const vector<double>&, const vector<array<double, 3>>&>(nominal_density, viscosity_approximation)
    {

    }

    /// @brief ���������� ��������
    /// @param grid_index 
    /// @param temperature 
    /// @return 
    double get_viscosity(size_t grid_index, double temperature) const {
        double result =
            viscosity_table_model_t::calc(viscosity_approximation[grid_index], temperature);
        return result;
    }

};

struct fluid_properties_t :
    fluid_properties_dynamic<double, double>,
    fluid_properties_static
{
    // ����� ��� ������� �� ������� �� ����������
};
