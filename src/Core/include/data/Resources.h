#pragma once

#include <iostream>

struct Resources
{
    double cpu;
    double ram;
    double disk;
    double bandwidth;
    double fpga;

    Resources(double cpu = 0, double ram = 0, double disk = 0, double bandwidth = 0, double fpga = 0)
        : cpu(cpu), ram(ram), disk(disk), bandwidth(bandwidth), fpga(fpga)
    {
    }

    Resources &operator+=(const Resources &rhs)
    {
        cpu += rhs.cpu;
        ram += rhs.ram;
        disk += rhs.disk;
        bandwidth += rhs.bandwidth;
        fpga += rhs.fpga;
        return *this;
    }

    Resources &operator-=(const Resources &rhs)
    {
        cpu -= rhs.cpu;
        ram -= rhs.ram;
        disk -= rhs.disk;
        bandwidth -= rhs.bandwidth;
        fpga -= rhs.fpga;
        return *this;
    }

    Resources operator+(const Resources &rhs) const
    {
        return Resources(cpu + rhs.cpu, ram + rhs.ram, disk + rhs.disk, bandwidth + rhs.bandwidth, fpga + rhs.fpga);
    }

    Resources operator-(const Resources &rhs) const
    {
        return Resources(cpu - rhs.cpu, ram - rhs.ram, disk - rhs.disk, bandwidth - rhs.bandwidth, fpga - rhs.fpga);
    }

    Resources operator*(double factor) const
    {
        return Resources(cpu * factor, ram * factor, disk * factor, bandwidth * factor, fpga * factor);
    }

    friend std::ostream &operator<<(std::ostream &os, const Resources &r)
    {
        os << "(" << r.cpu << ", " << r.ram << ", " << r.disk << ", " << r.bandwidth << ", " << r.fpga << ")";
        return os;
    }

    bool operator==(const Resources &rhs) const
    {
        return cpu == rhs.cpu && ram == rhs.ram && disk == rhs.disk && bandwidth == rhs.bandwidth && fpga == rhs.fpga;
    }

    bool operator!=(const Resources &rhs) const
    {
        return !(*this == rhs);
    }

    Resources operator/(const Resources &rhs) const
    {
        return Resources(cpu / rhs.cpu, ram / rhs.ram, disk / rhs.disk, bandwidth / rhs.bandwidth, fpga / rhs.fpga);
    }

    Resources operator/(double factor) const
    {
        return Resources(cpu / factor, ram / factor, disk / factor, bandwidth / factor, fpga / factor);
    }

    Resources operator/=(double factor)
    {
        cpu /= factor;
        ram /= factor;
        disk /= factor;
        bandwidth /= factor;
        fpga /= factor;
        return *this;
    }
};

inline bool canHost(const Resources &request, const Resources &available)
{
    return request.cpu <= available.cpu && request.ram <= available.ram && request.disk <= available.disk && request.bandwidth <= available.bandwidth && request.fpga <= available.fpga;
}