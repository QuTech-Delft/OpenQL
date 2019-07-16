/**
 * @file   transmon_driver.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  transmon driver
 */
#ifndef TRANSMON_H
#define TRANSMON_H

#include "circuit.h"

#include "platform.h"

namespace ql
{
class transmon_driver
{

public:

    transmon_driver()
    {
    }

    int compile(circuit& c, std::string file_name, bool optimize=false)
    {
        circuit dc = decompose(c);
        return 0;
    }

protected:

    circuit decompose(circuit& c)
    {
        circuit dc = c;
        return c;
    }

};

}

#endif // TRANSMON_H
