#pragma once


/// @brief �������������� ������������� �� ����������
/// \param reynolds_number
/// \param relative_roughness
/// \return
inline double hydraulic_resistance_shifrinson(double reynolds_number, double relative_roughness)
{
    return 0.11 * pow(relative_roughness, 0.25);
}

/// @brief �������������� ������������� �� ���������
/// @param reynolds_number 
/// @param relative_roughness 
/// @return 
inline double hydraulic_resistance_altshul(double reynolds_number, double relative_roughness)
{
    return 0.11 * pow(relative_roughness + 68 / reynolds_number, 0.25);
}


/// @brief ������ ��������������� ������������� � ������� ��������� ����� ����������
/// ������������ ������ �� ������� ������ [��������, ������, �-�� (1)]
/// @param reynolds_number 
/// @param relative_roughness 
/// @return 
inline double hydraulic_resistance_isaev(double reynolds_number, double relative_roughness) {
    const double Re = fabs(reynolds_number);
    const double& Ke = relative_roughness;

    double lam;

    // ������� �� ��-75.180.00-���-258-10. 
    // � ����� ��� ������ ��� ������� ����� ����������

    if (Re < 1) {
        lam = 64; // ����� ��� Re = 1
    }
    else if (Re < 2320)
    {
        lam = 64 / Re; // ����� 
    }
    else if (Re < 4000)
    {
        // ����� + �������, ���������� �������
        double gm = 1 - exp(-0.002 * (Re - 2320));
        lam = 64 / Re * (1 - gm) + 0.3164 / pow(Re, 0.25) * gm;
    }
    //else if(Re < 1e4)//min(1e5,27/pow(Ke,1.143))  27/pow(Ke,1.143)   1e4   //10/Ke
    //{
    //    lam=0.3164/pow(Re,0.25);
    //}
    else if (Re < 560 / Ke)
    {
        // ����� �� [��������, ������], �-�� (1)
        lam = 1.0 / sqr(-1.8 * log10(6.8 / Re + pow(Ke / 3.7, 1.1)));
    }
    else
    {
        // ���������
        lam = 0.11 * pow(Ke, 0.25);
    }

    return lam;
}


