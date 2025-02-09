/*
  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
#ifndef ECL_HDF5_SERIALIZER_HH
#define ECL_HDF5_SERIALIZER_HH

#include <opm/common/utility/Serializer.hpp>

#include <opm/simulators/utils/HDF5File.hpp>
#include <opm/simulators/utils/moduleVersion.hpp>
#include <opm/simulators/utils/ParallelCommunication.hpp>
#include <opm/simulators/utils/SerializationPackers.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace Opm {

//! \brief Class for (de-)serializing using HDF5.
class HDF5Serializer : public Serializer<Serialization::MemPacker> {
public:
    HDF5Serializer(const std::string& fileName,
                   HDF5File::OpenMode mode,
                   Parallel::Communication comm)
        : Serializer<Serialization::MemPacker>(m_packer_priv)
        , m_h5file(fileName, mode, comm)
    {}

    //! \brief Serialize and write data to restart file.
    //! \tparam T Type of class to write
    //! \param data Class to write restart data for
    template<class T>
    void write(T& data,
               const std::string& group,
               const std::string& dset,
               HDF5File::DataSetMode mode = HDF5File::DataSetMode::PROCESS_SPLIT)
    {
        try {
            this->pack(data);
        } catch (...) {
            m_packSize = std::numeric_limits<size_t>::max();
            throw;
        }

        m_h5file.write(group, dset, m_buffer, mode);
    }

    //! \brief Writes a header to the file.
    //! \param simulator_name Name of simulator used
    //! \param module_version Version of simulator used
    //! \param time_stamp Build time-stamp for simulator used
    //! \param case_name Name of case file is associated with
    //! \param params List of parameter values
    //! \param num_procs Number of processes used
    void writeHeader(const std::string& simulator_name,
                     const std::string& module_version,
                     const std::string& time_stamp,
                     const std::string& case_name,
                     const std::string& params,
                     int num_procs)
    {
        try {
            this->pack(simulator_name, module_version, time_stamp,
                       case_name, params, num_procs);
        } catch (...) {
            m_packSize = std::numeric_limits<size_t>::max();
            throw;
        }
        m_h5file.write("/", "simulator_info", m_buffer, HDF5File::DataSetMode::ROOT_ONLY);
    }

    //! \brief Read data and deserialize from restart file.
    //! \tparam T Type of class to read
    //! \param data Class to read restart data for
    template<class T>
    void read(T& data,
              const std::string& group,
              const std::string& dset,
              HDF5File::DataSetMode mode = HDF5File::DataSetMode::PROCESS_SPLIT)
    {
        m_h5file.read(group, dset, m_buffer, mode);
        this->unpack(data);
    }

    //! \brief Returns the last report step stored in file.
    int lastReportStep() const
    {
        const auto entries = m_h5file.list("/report_step");
        int last = -1;
        for (const auto& entry : entries) {
            int num = std::atoi(entry.c_str());
            last = std::max(last, num);
        }

        return last;
    }

    //! \brief Returns a list of report steps stored in restart file.
    std::vector<int> reportSteps() const
    {
        const auto entries = m_h5file.list("/report_step");
        std::vector<int> result(entries.size());
        std::transform(entries.begin(), entries.end(), result.begin(),
                       [](const std::string& input)
                       {
                          return std::atoi(input.c_str());
                       });
        std::sort(result.begin(), result.end());
        return result;
    }

private:
    const Serialization::MemPacker m_packer_priv{}; //!< Packer instance
    HDF5File m_h5file; //!< HDF5 backend for the serializer
};

}

#endif
