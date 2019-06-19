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
            size_t start_cycle;                         // start cycle for all gates in parallel_sections
            size_t duration_in_cycles;                  // the maximum gate duration in parallel_sections
            std::list<section_t> parallel_sections;
        };

        typedef std::list<bundle_t>bundles_t;           // note that subsequent bundles can overlap in time

        std::string qasm(bundles_t & bundles)
        {
            std::stringstream ssqasm;
            size_t curr_cycle=1;

            ssqasm << '\n';
            for (bundle_t & abundle : bundles)
            {
                auto st_cycle = abundle.start_cycle;
                auto delta = st_cycle - curr_cycle;
                // DOUT("Printing bundle with st_cycle: " << st_cycle);
                if(delta>1)
                    ssqasm << "    wait " << delta-1 << '\n';
                // else
                //   ssqasm << '\n';

                auto ngates = 0;
                for( auto sec_it = abundle.parallel_sections.begin(); sec_it != abundle.parallel_sections.end(); ++sec_it )
                {
                    ngates += sec_it->size();
                }
                ssqasm << "    ";
                if (ngates > 1) ssqasm << "{ ";
                auto isfirst = 1;
                for( auto sec_it = abundle.parallel_sections.begin(); sec_it != abundle.parallel_sections.end(); ++sec_it )
                {
                    for ( auto gp : (*sec_it))
                    {
                        if (isfirst == 0)
                            ssqasm << " | ";
                        ssqasm << gp->qasm();
                        isfirst = 0;
                    }
                }
                if (ngates > 1) ssqasm << " }";
                curr_cycle+=delta;
                ssqasm << "\n";
            }

            if( !bundles.empty() )
            {
                auto & last_bundle = bundles.back();
                int lsduration = last_bundle.duration_in_cycles;
                if( lsduration > 1 )
                    ssqasm << "    wait " << lsduration -1 << '\n';
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
