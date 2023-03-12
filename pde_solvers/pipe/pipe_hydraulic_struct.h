#pragma once



/// @brief �������� ������� �����
struct PipeProfile {
    /// @brief ������������ �������, �
    vector<double> coordinates;
    /// @brief �������� �������, �
    vector<double> heights;
    /// @brief ������� �����������, ��
    vector<double> capacity;
    /// @brief ����� ������� �����, �
    double getLength() const
    {
        return coordinates.back() - coordinates.front();
    }
    /// @brief ���������� ������ ��������� �����
    size_t getPointCount() const
    {
        return coordinates.size();
    }

    static PipeProfile create(size_t segment_count,
        double x_begin, double x_end, double z_begin, double z_end,
        double capacity)
    {
        size_t n = segment_count + 1;
        PipeProfile result;
        result.coordinates = result.heights = vector<double>(n);
        result.capacity = vector<double>(n, capacity);
        double length = x_end - x_begin;
        double dx = length / segment_count;
        for (size_t index = 0; index < n; ++index) {
            double x = x_begin + dx * index;
            result.coordinates[index] = x;

            double alpha = 1 - x / length;
            double z = z_begin * alpha + z_end * (1 - alpha);
            result.heights[index] = z;
        }
        return result;
    }
};

/// @brief ��������� ������ �����
struct pipe_wall_model_t {
    /// @brief ���������� �������, �
    double diameter{ 0.8 };
    /// @brief ������� ������, �
    double wallThickness{ 10e-3 }; // 10 ��
    /// @brief ������������� �������������, � (�� � ��!)
    double equivalent_roughness{ 0.125e-3 }; // ������������� �� ��
    /// @brief ����������� ��������, ��� ������� ������������ �������, ��
    double nominal_pressure{ 101325 }; // ��� ���� ����������� �������� ��� ����� ����������?
    /// @brief ������ ��������� ���� ��������� ���� 
    /// (��������: ����� 2017, ���. 92)
    double wall_elasticity_modulus{ 2e11 };
    /// @brief ����������� ��������� ��������
    /// �������� �� ��������� ����� �� [����� 2017], ���. 93
    double poissonStrain{ 0.28 };

    /// @brief ����������� ����������� ��� ����� (1/��)
    /// �����������, ����������� ��������� ������� ������� ��� ���������� �������� �� ������������
    /// � ���������� ���������� ��� \beta_S
    double getCompressionRatio() const {
        return diameter * (1 - poissonStrain * poissonStrain) / (wall_elasticity_modulus * wallThickness);
    }
    /// @brief ������������� �������������
    double relativeRoughness() const {
        return equivalent_roughness / diameter;
    }
    /// @brief ������� ������� \S_0, �^2
    double getArea() const
    {
        return (M_PI * diameter * diameter) / 4;
    }

    /// @brief ������� ������� \S, ��������� �� ��������, �^2
    /// \param oil
    /// \param area
    /// \param pressure
    /// \return
    double getPressureArea(const OilParameters& oil, double nominalArea, double pressure) const
    {
        double beta_rho = getCompressionRatio();
        double multiplier = (1 + beta_rho * (pressure - nominal_pressure));
        return nominalArea * multiplier;
    }
};

/// @brief ��������� ��������� (� �������� ����� ���� ���!!)
struct adaptation_parameters {
    /// @brief ��������� ������ ��� ��������� ������� ������
    double friction{ 1 };
    /// @brief �������� �� ��������� ������������
    double diameter{ 1 };
    /// @brief �������� �� ����������� ����������� ������
    double heat_capacity{ 1 };
};


/// @brief �������� �����
template <typename AdaptationParameters>
struct pipe_properties_t
{
    /// @brief ������� ��� ������� (�����). �� �������� �������
    PipeProfile profile;
    /// @brief ������ ��� ������� ������
    pipe_wall_model_t wall;

    /// @brief ��������� ���������
    AdaptationParameters adaptation;


    double(*resistance_function)(double, double) { hydraulic_resistance_isaev };


    /// @brief �������� ����� � ��������, �^2/�
    double getSoundVelocity(const OilParameters& oil) const
    {
        double beta_S = wall.getCompressionRatio();
        double beta_rho = oil.density.getCompressionRatio();
        double c = sqrt(1 / (oil.density() * (beta_S + beta_rho)));
        return c;
    }

    /// @brief �������� ����� � ��������
    double get_sound_velocity(double beta_rho, double nominal_density) const
    {
        double beta_S = wall.getCompressionRatio();
        double c = sqrt(1 / (nominal_density * (beta_S + beta_rho)));
        return c;
    }

    /// @brief ������������� ������� ������� \A, ������� �����, ����� ��������� �������� ��� ����������� ��������, �^2
    /// \param oil
    /// \param pressureArea
    /// \param density
    /// \return
    double getNominalArea(OilParameters oil, double pressureArea, double density) const
    {
        return (density / oil.density()) * pressureArea;
    }
    /// @brief \teta
    /// \param oil
    /// \param pressure
    /// \return
    double getTeta(OilParameters oil, double pressure) const
    {
        return 1 + (oil.density.getCompressionRatio() + wall.getCompressionRatio())
            * (pressure - wall.nominal_pressure);
    }
};

typedef pipe_properties_t<adaptation_parameters> PipeProperties;
