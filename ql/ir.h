/**
 * @file   ir.h
 * @date   02/2018
 * @author Imran Ashraf
 * @brief  common IR implementation
 */

#ifndef IR_H
#define IR_H

#include "gate.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <list>

namespace ql
{
    namespace ir
    {
        typedef std::list<ql::gate *>section_t;
        class bundle_t
        {
        public:
            size_t start_cycle;
            size_t duration_in_cycles;
            std::list<section_t> parallel_sections;
        };

        typedef std::list<bundle_t>bundles_t;

        std::string qasm(bundles_t & bundles)
        {
            std::stringstream ssqasm;
            size_t curr_cycle=1;

            for (bundle_t & abundle : bundles)
            {
                auto st_cycle = abundle.start_cycle;
                auto delta = st_cycle - curr_cycle;
                if(delta>1)
                    ssqasm << "\n    wait " << delta-1 << '\n';
                else
                    ssqasm << '\n';

                ssqasm << "    ";
                if (abundle.parallel_sections.size() > 1) ssqasm << "{ ";
                for( auto sec_it = abundle.parallel_sections.begin();
                     sec_it != abundle.parallel_sections.end(); ++sec_it )
                {
                    auto first_ins_it = sec_it->begin();
                    auto insqasm = (*(first_ins_it))->qasm();
                    ssqasm << insqasm;

                    if( std::next(sec_it) != abundle.parallel_sections.end() )
                    {
                        ssqasm << " | ";
                    }
                }
                if (abundle.parallel_sections.size() > 1) ssqasm << " }";
                curr_cycle+=delta;
            }

            if( !bundles.empty() )
            {
                auto & last_bundle = bundles.back();
                int lsduration = last_bundle.duration_in_cycles;
                if( lsduration > 1 )
                    ssqasm << "\n    wait " << lsduration -1 << '\n';
            }

            return ssqasm.str();
        }

        void write_qasm(bundles_t & bundles)
        {
            std::ofstream fout;
            std::string fname( ql::options::get("output_dir") + "/ir.qasm" );
            fout.open( fname, std::ios::binary);
            if ( fout.fail() )
            {
                EOUT("Error opening file " << fname << std::endl
                         << "Make sure the output directory ("<< ql::options::get("output_dir") << ") exists");
                return;
            }

            fout << qasm(bundles);
            fout.close();
        }
    } // namespace ir
} //namespace ql

#endif
