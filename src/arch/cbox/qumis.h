/**
 * @file   qumis.h
 * @date   07/2017
 * @author Nader Khammassi
 * @brief  qumis code emitter
 */

#ifndef QL_QUMIS_H
#define QL_QUMIS_H

#include <string>
#include <bitset>

#include <arch/cbox/instruction_scheduler.h>

namespace ql
{
   namespace arch
   {
      #define __trigger_width__  (8)  //  7 out of the 8 markers can be used
      #define __lut_id_width__   (4)  //  pulse instruction : pulse is 4 bits
      #define __pulse_cw_width__ (3)  //  external trig : pulse id is 3 bits
      #define __awg_number__     (3)
      #define __resources__    \
              (__trigger_width__+__awg_number__)

      const std::string channel_name[] = {"TRIG_0","TRIG_1","TRIG_2","TRIG_3","TRIG_4","TRIG_5","TRIG_6","TRIG_7","AWG_0","AWG_1","AWG_2"};

      // single qumis instruction
      // typedef size_t               channel_t;
      typedef bool                 bit_t;
      typedef std::string          qumis_instr_t;
      typedef std::vector<bit_t>   bitset_t;

      class qumis_instruction;

      typedef std::vector<qumis_instruction *> qumis_program_t;

      typedef std::vector<size_t>  instr_schedule_t;


      // instruction type
      typedef enum
      {
         __qumis_trigger__       ,
         __qumis_pulse__         ,
         __qumis_cw_trigger__    ,
         __qumis_pulse_trigger__ ,
         __qumis_trigger_seq__   ,
         __qumis_readout__       ,
         __qumis_buffer__        ,
         __qumis_wait__
      } qumis_instr_type_t;

      // operation type
      typedef enum
      {
         __none__       ,
         __rf__         ,
         __flux__       ,
         __measurement__,
         __wait__       ,
         __unknown_operation__
      } operation_type_t;

      #define __operation_types_num__ (4)

      // pulse/trigger codewords
      typedef std::bitset<__lut_id_width__>   pulse_id_t;
      typedef std::bitset<__pulse_cw_width__> pulse_cw_t;
      typedef std::bitset<__trigger_width__>  codeword_t;

      // hardware resources
      typedef std::bitset<__resources__>      resources_t;
      typedef std::vector<size_t>             qubit_set_t;

      /**
       * qumis instruction interface
       */
      class qumis_instruction
      {
         public:

            resources_t         used_resources;
            qubit_set_t         used_qubits;

            size_t              duration;
            size_t              latency = 0;
            size_t              start = 0;

            qumis_instr_type_t  instruction_type;
            operation_type_t    operation_type;

            std::string         qasm_label;

            bool                latency_compensated = false;

         public:

            /**
             * emit qumis code
             */
            virtual qumis_instr_t code() = 0;

            /**
             * return instruction trace
             */
            virtual instruction_traces_t trace() = 0;

            /**
             *  compensate for latency
             */
            virtual void compensate_latency()
            {
               if (!latency_compensated)
               {
                  // println("compensate latency : " << start << " -> " << (start-latency) << " : latency = " << latency);
                  start -= latency;
                  latency_compensated = true;
               }
               else
               {
                  // println("[x] warning : latency of instruction '" << this << "' is already compensated !");
                  WOUT("latency of instruction '" << this << "' is already compensated !");
               }
            }

            /**
             * set used_qubits
             */
            virtual void set_used_qubits(qubit_set_t& uq)
            {
               used_qubits = uq;
            }


            /**
             * set start
             */
            virtual void set_start(size_t t)
            {
               start = t;
            }

            /**
             * decompose meta-instructions
             */
            virtual qumis_program_t decompose()
            {
               qumis_program_t p;
               p.push_back(this);
               return p;
            }

            /**
             * return qumis instruction type
             */
            virtual qumis_instr_type_t get_instruction_type()
            {
               return instruction_type;
            }

            /**
             * return operation type
             */
            virtual operation_type_t get_operation_type()
            {
               return operation_type;
            }

      };

      /**
       * pulse
       */
      class pulse : public qumis_instruction
      {
         public:

            size_t codeword; // lut id
            size_t awg;

         public:

            /**
             * ctor
             */
            pulse(size_t codeword, size_t awg, size_t duration, ql::arch::operation_type_t operation_type, size_t latency=0) : codeword(codeword), awg(awg)
            {
               this->operation_type    = operation_type ;
               this->instruction_type  = __qumis_pulse__;
               this->duration          = duration;
               this->latency           = latency;
               latency_compensated     = false;
               used_resources.set(__trigger_width__+awg);
            }

            /**
             * generate code
             */
            qumis_instr_t code()
            {
               std::stringstream params;
               pulse_id_t pid = codeword;
               pid.set(__lut_id_width__-1);
               params << (awg == 0 ? pid : 0) << ", " << (awg == 1 ? pid : 0) << ", " << (awg == 2 ? pid : 0); // << "\nwait " << duration;
               qumis_instr_t instr = "pulse " + params.str();
               // println("[i] used resources : " << used_resources);
               return instr;
            }

            /**
             * trace
             */
            instruction_traces_t trace()
            {
               instruction_traces_t trs;
               size_t latent_start = (latency_compensated ? (start) : (start-latency));
               std::string label   = qasm_label + " : " + code();
               // println("pulse label : " << label);
               instruction_trace_t t  = { (__trigger_width__+awg), label, start, (start+duration), "#4567aa", __top_pos__};
               // instruction_trace_t tl = { (__trigger_width__+awg), "", start-latency, (start-latency+duration), "#403377", __bottom_pos__ }; // latent
               instruction_trace_t tl = { (__trigger_width__+awg), label, latent_start, latent_start+duration, "#808080", __bottom_pos__ }; // latent
               trs.push_back(t);
               trs.push_back(tl);
               return trs;
            }
#if 0
            /**
             * instruction_type
             */
            qumis_instr_type_t instruction_type()
            {
               return __qumis_pulse__;
            }
#endif
      };

      /**
       * trigger
       */
      class trigger : public qumis_instruction
      {
         public:

            codeword_t codeword;

         public:

            /**
             * ctor
             */
            trigger(codeword_t codeword, size_t duration, ql::arch::operation_type_t operation_type, size_t latency=0) : codeword(codeword)
            {
               this->operation_type    = operation_type   ;
               this->instruction_type  = __qumis_trigger__;
               this->duration          = duration;
               this->latency           = latency;
               used_resources          = codeword.to_ulong();
               latency_compensated     = false;
            }

            /**
             * generate code
             */
            qumis_instr_t code()
            {
               std::stringstream params;
               std::bitset<__trigger_width__> cwr = codeword.to_ulong();
               // println("codeword  :\t" << codeword);
               std::bitset<7> cw  = cwr.to_ulong();
               // params << codeword << ", " << duration; // << "\nwait " << duration;
               params << cw << ", " << duration; // << "\nwait " << duration;
               qumis_instr_t instr = "trigger " + params.str();
               return instr;
            }

            /**
             * trace
             */
            instruction_traces_t trace()
            {
               instruction_traces_t trs;
               std::string label   = qasm_label + " : " + code();
               // println("trig label : " << label);
               for (size_t ch=0; ch<codeword.size(); ++ch)
               {
                  if (codeword.test(ch))
                  {
                     size_t latent_start = (!latency_compensated ? (start) : (start-latency));
                     instruction_trace_t t  = { ch, label, start, (start+duration), "#c467aa", __top_pos__ };
                     instruction_trace_t lt = { ch, label, latent_start, (latent_start+duration), "#808080", __bottom_pos__ };
                     trs.push_back(lt);
                     trs.push_back(t);
                     // println("qasm label   : " << label);
                     // println("latency      : " << latency);
                     // println("start        : " << start);
                     // println("latent start : " << latent_start);
                  }
               }
               return trs;
            }

      };

      /**
       * wait
       */
      class wait : public qumis_instruction
      {
         public:

            size_t ch;

         public:

            /**
             * ctor
             */
            wait(size_t ch, size_t duration, ql::arch::operation_type_t operation_type, size_t latency=0) : ch(ch)
            {
               this->operation_type    = operation_type ;
               this->instruction_type  = __qumis_wait__;
               this->duration          = duration;
               this->latency           = latency;
               latency_compensated     = false;
               used_resources.set(ch);
            }

            /**
             * generate code
             */
            qumis_instr_t code()
            {
               qumis_instr_t instr;
               // instr = "wait " + std::string(duration);
               // println("[i] used resources : " << used_resources);
               return instr;
            }

            /**
             * trace
             */
            instruction_traces_t trace()
            {
               instruction_traces_t trs;
               size_t latent_start = (latency_compensated ? (start) : (start-latency));
               instruction_trace_t t  = { (ch), "buffer", start, (start+duration), "#ff9933", __top_pos__};
               // instruction_trace_t tl = { (__trigger_width__+awg), "", start-latency, (start-latency+duration), "#403377", __bottom_pos__ }; // latent
               instruction_trace_t tl = { (ch), "buffer", latent_start, latent_start+duration, "#808080", __bottom_pos__ }; // latent
               trs.push_back(t);
               trs.push_back(tl);
               return trs;
            }

      };

      /**
       * codeword_trigger
       */
      class codeword_trigger : public qumis_instruction
      {
         public:

            codeword_t codeword;
            size_t     ready_bit;
            size_t     ready_bit_duration;

            // decompose
            qumis_program_t instructions;

         public:

            /**
             * ctor
             */
            codeword_trigger(codeword_t codeword, size_t duration,
                  size_t ready_bit, size_t ready_bit_duration,
                  ql::arch::operation_type_t operation_type, size_t latency=0, std::string qasm_label="") : codeword(codeword), ready_bit(ready_bit), ready_bit_duration(ready_bit_duration)
         {
            this->operation_type    = operation_type   ;
            this->instruction_type  = __qumis_cw_trigger__;
            this->duration          = duration;
            this->latency           = latency;
            used_resources          = codeword.to_ulong();
            used_resources.set(ready_bit);
            if (ready_bit_duration > (duration-1))
               println("[x] error in codeword trigger definition : 'ready_bit_duration' cannot be greater than overall 'duration' !");

            codeword_t ready_cw = 0;
            ready_cw.set(7-ready_bit);
            trigger * rdb = new trigger(ready_cw,ready_bit_duration,operation_type,latency);
            trigger * cwt = new trigger(codeword,duration,operation_type,latency);
            rdb->qasm_label = qasm_label;
            cwt->qasm_label = qasm_label;
            instructions.push_back(cwt);
            instructions.push_back(rdb);
         }

            /**
             * decompose codeword trigger
             */
            qumis_program_t decompose()
            {
               instructions[0]->start   = start;
               instructions[0]->latency = latency;
               instructions[1]->start   = start+1;
               instructions[1]->latency = latency;
               // println("codeword_trigger latency : " << instructions[0]->latency);
               // println("codeword_trigger start   : " << instructions[0]->start);
               // println("ready_bit        latency : " << instructions[1]->latency);
               // println("codeword_trigger start   : " << instructions[1]->start);
               return instructions;
            }

            /**
             * set used qubits
             */
            void set_used_qubits(qubit_set_t& uq)
            {
               used_qubits = uq;
               for (auto i : instructions)
                  i->used_qubits = uq;
            }

            /**
             *  compensate for latency
             */
            void compensate_latency()
            {
               if (!latency_compensated)
               {
                  // println("compensate latency : " << start << " -> " << (start-latency) << " : latency = " << latency);
                  start -= latency;
                  instructions[0]->compensate_latency();
                  instructions[1]->compensate_latency();
                  latency_compensated = true;
               }
               else
               {
                  println("[x] warning : latency of instruction '" << this << "' is already compensated !");
               }
            }


            /**
             * generate code
             */
            qumis_instr_t code()
            {
               codeword_t ready_cw = 0;
               ready_cw.set(7-ready_bit);
               std::stringstream instr;
               instr << "trigger " << codeword << ", " << duration << "\nwait 1\n";
               instr << "trigger " << ready_cw << ", " << ready_bit_duration; //  << "\nwait " << (duration-1);
               // println("[i] used resources : " << used_resources);
               return instr.str();
            }

            /**
             * trace
             */
            instruction_traces_t trace()
            {
               instruction_traces_t trs;
               size_t latent_start = (latency_compensated ? (start) : (start-latency));
               std::string label   = qasm_label + " : " + code();
               // println("cw label : " << label);
               for (size_t ch=0; ch<codeword.size(); ++ch)
               {
                  if (codeword.test(ch))
                  {
                     instruction_trace_t t  = { ch, label, start, (start+duration), "#DD5437", __top_pos__ };
                     instruction_trace_t lt = { ch, label, latent_start, (latent_start+duration), "#808080", __bottom_pos__  };
                     trs.push_back(lt);
                     trs.push_back(t);
                  }
               }
               instruction_trace_t t = { ready_bit, label, start+1, (start+1+ready_bit_duration), "#DD5437", __top_pos__ };
               instruction_trace_t lt = { ready_bit, label, latent_start+1, (latent_start+1+ready_bit_duration), "#808080", __bottom_pos__ };
               trs.push_back(lt);
               trs.push_back(t);
               return trs;
            }

            /**
             * set start
             */
            void set_start(size_t t)
            {
               start = t;
               instructions[0]->set_start(t);
               instructions[1]->set_start(t+1);
            }

      };


      /**
       * pulse_trigger
       *   external awg trigger
       */
      class pulse_trigger : public qumis_instruction
      {
         public:

            pulse_cw_t codeword;
            size_t     trig_channel;

            // decompose
            qumis_program_t  instructions;
            instr_schedule_t instr_schedule;

         public:

            /**
             * ctor
             */
            pulse_trigger(pulse_cw_t codeword, size_t trig_channel, size_t duration,
                  ql::arch::operation_type_t operation_type,
                  size_t latency=0, std::string qasm_label="") : codeword(codeword),
                                                                 trig_channel(trig_channel)
            {
               this->operation_type    = operation_type   ;
               this->instruction_type  = __qumis_pulse_trigger__;
               this->duration          = duration;
               this->latency           = latency;
               used_resources.set(trig_channel);

               if (codeword.to_ulong() > 7)
                  println("[x] error : codeword cannot be greater than 7 (3 bits) !");

               codeword_t trig_mask;
               trig_mask.set(7-trig_channel);

               // build external codeword trigger
               size_t cw = codeword.to_ulong();
               switch (cw)
               {
                  case 0 :
                  {
                     trigger * t0 = new trigger(trig_mask,1,operation_type,latency);
                     // wait    * w0 = new wait(trig_channel,4,operation_type,latency);
                     t0->qasm_label = qasm_label;
                     // w0->qasm_label = qasm_label;
                     instructions.push_back(t0);
                     // instructions.push_back(w0);
                     instr_schedule.push_back(0);
                     break;
                  }
                  case 1 :
                  {
                     trigger * t0 = new trigger(trig_mask,1,operation_type,latency);
                     // wait    * w0 = new wait(trig_channel,3,operation_type,latency);
                     trigger * t1 = new trigger(trig_mask,1,operation_type,latency);
                     // wait    * w1 = new wait(trig_channel,1,operation_type,latency);
                     t0->qasm_label = qasm_label;
                     // w0->qasm_label = qasm_label;
                     t1->qasm_label = qasm_label;
                     // w1->qasm_label = qasm_label;
                     instructions.push_back(t0);
                     // instructions.push_back(w0);
                     instructions.push_back(t1);
                     // instructions.push_back(w1);
                     instr_schedule.push_back(0);
                     instr_schedule.push_back(3);
                     break;
                  }
                  case 2 :
                  {
                     trigger * t0 = new trigger(trig_mask,1,operation_type,latency);
                     // wait    * w0 = new wait(trig_channel,2,operation_type,latency);
                     trigger * t1 = new trigger(trig_mask,1,operation_type,latency);
                     // wait    * w1 = new wait(trig_channel,2,operation_type,latency);
                     t0->qasm_label = qasm_label;
                     // w0->qasm_label = qasm_label;
                     t1->qasm_label = qasm_label;
                     // w1->qasm_label = qasm_label;
                     instructions.push_back(t0);
                     // instructions.push_back(w0);
                     instructions.push_back(t1);
                     // instructions.push_back(w1);
                     instr_schedule.push_back(0);
                     instr_schedule.push_back(2);
                     break;
                  }
                  case 3 :
                  {
                     trigger * t0 = new trigger(trig_mask,1,operation_type,latency);
                     // wait    * w0 = new wait(trig_channel,2,operation_type,latency);
                     trigger * t1 = new trigger(trig_mask,2,operation_type,latency);
                     // wait    * w1 = new wait(trig_channel,2,operation_type,latency);
                     t0->qasm_label = qasm_label;
                     // w0->qasm_label = qasm_label;
                     t1->qasm_label = qasm_label;
                     // w1->qasm_label = qasm_label;
                     instructions.push_back(t0);
                     // instructions.push_back(w0);
                     instructions.push_back(t1);
                     // instructions.push_back(w1);
                     instr_schedule.push_back(0);
                     instr_schedule.push_back(2);
                     break;
                  }
                  case 4 :
                  {
                     trigger * t0 = new trigger(trig_mask,2,operation_type,latency);
                     // wait    * w0 = new wait(trig_channel,4,operation_type,latency);
                     t0->qasm_label = qasm_label;
                     // w0->qasm_label = qasm_label;
                     instructions.push_back(t0);
                     // instructions.push_back(w0);
                     instr_schedule.push_back(0);
                     break;
                  }
                  case 5 :
                  {
                     trigger * t0 = new trigger(trig_mask,2,operation_type,latency);
                     // wait    * w0 = new wait(trig_channel,3,operation_type,latency);
                     trigger * t1 = new trigger(trig_mask,1,operation_type,latency);
                     // wait    * w1 = new wait(trig_channel,1,operation_type,latency);
                     t0->qasm_label = qasm_label;
                     // w0->qasm_label = qasm_label;
                     t1->qasm_label = qasm_label;
                     // w1->qasm_label = qasm_label;
                     instructions.push_back(t0);
                     // instructions.push_back(w0);
                     instructions.push_back(t1);
                     // instructions.push_back(w1);
                     instr_schedule.push_back(0);
                     instr_schedule.push_back(3);
                     break;
                  }
                  case 6 :
                  {
                     trigger * t0 = new trigger(trig_mask,3,operation_type,latency);
                     // wait    * w0 = new wait(trig_channel,4,operation_type,latency);
                     t0->qasm_label = qasm_label;
                     // w0->qasm_label = qasm_label;
                     instructions.push_back(t0);
                     // instructions.push_back(w0);
                     instr_schedule.push_back(0);
                     break;
                  }
                  case 7 :
                  {
                     trigger * t0 = new trigger(trig_mask,4,operation_type,latency);
                     // wait    * w0 = new wait(trig_channel,4,operation_type,latency);
                     t0->qasm_label = qasm_label;
                     // w0->qasm_label = qasm_label;
                     instructions.push_back(t0);
                     //instructions.push_back(w0);
                     instr_schedule.push_back(0);
                     break;
                  }
               }
            }

            /**
             * decompose codeword trigger
             */
            qumis_program_t decompose()
            {
               // instructions[0]->start   = start;
               // instructions[0]->latency = latency;
               // instructions[1]->start   = start+1;
               // instructions[1]->latency = latency;
               for (size_t i=0; i<instructions.size(); ++i)
               {
                  instructions[i]->start   = start+instr_schedule[i];
                  instructions[i]->latency = latency;
               }
               // println("pulse_trigger latency : " << instructions[0]->latency);
               // println("pulse_trigger start   : " << instructions[0]->start);
               // println("ready_bit        latency : " << instructions[1]->latency);
               // println("pulse_trigger start   : " << instructions[1]->start);
               return instructions;
            }

            /**
             *  compensate for latency
             */
            void compensate_latency()
            {
               if (!latency_compensated)
               {
                  // println("compensate latency : " << start << " -> " << (start-latency) << " : latency = " << latency);
                  start -= latency;
                  for (auto i : instructions)
                     i->compensate_latency();
                  latency_compensated = true;
               }
               else
               {
                  println("[x] warning : latency of instruction '" << this << "' is already compensated !");
               }
            }

            /**
             * set used qubits
             */
            void set_used_qubits(qubit_set_t& uq)
            {
               used_qubits = uq;
               for (auto i : instructions)
                  i->used_qubits = uq;
            }



            /**
             * generate code
             */
            qumis_instr_t code()
            {
               std::stringstream instr;
               // instr << "trigger " << codeword << ", " << duration << "\nwait 1\n";
               // instr << "trigger " << ready_cw << ", " << ready_bit_duration; //  << "\nwait " << (duration-1);
               for (size_t i=0; i<instructions.size(); ++i)
               {
                  if (instr_schedule[i])
                     instr << "wait " << instr_schedule[i] << "\n";
                  instr << instructions[i]->code();
               }
               // println("[i] used resources : " << used_resources);
               return instr.str();
            }

            /**
             * trace
             */
            instruction_traces_t trace()
            {
               instruction_traces_t trs;
               size_t latent_start = (latency_compensated ? (start) : (start-latency));
               std::string label   = qasm_label + " : " + code();
               // println("qasm label   : " << label);
               // println("start        : " << start);
               // println("latent start : " << latent_start);

               for (size_t i=0; i<instructions.size(); ++i)
               {
                  instruction_trace_t t  = { trig_channel, label, start+instr_schedule[i], (start+instr_schedule[i]+instructions[i]->duration), "#DD5437", __top_pos__ };
                  instruction_trace_t lt = { trig_channel, label, latent_start+instr_schedule[i], (latent_start+instr_schedule[i]+instructions[i]->duration), "#808080", __bottom_pos__ };
                  trs.push_back(lt);
                  trs.push_back(t);
               }
               return trs;
            }

            /**
             * set start
             */
            void set_start(size_t t)
            {
               start = t;

               for (size_t i=0; i<instructions.size(); ++i)
                  instructions[i]->start   = t+instr_schedule[i];
            }

      };


      /**
       * trigger_sequence
       *   trigger sequence
       */
      class trigger_sequence : public qumis_instruction
      {
         public:

            size_t     trig_channel;
            size_t     trig_width;

            qumis_program_t  instructions;

         public:

            /**
             * ctor
             */
            trigger_sequence(size_t trig_channel, size_t trig_width, size_t duration,
                  ql::arch::operation_type_t operation_type,
                  size_t latency=0, std::string qasm_label="") : trig_channel(trig_channel), trig_width(trig_width)

            {
               this->operation_type    = operation_type   ;
               this->instruction_type  = __qumis_trigger_seq__;
               this->duration          = duration;
               this->latency           = latency;
               this->start             = 0;

               used_resources.set(trig_channel);

               // already tested
               // if (trig_channel > 7)
                  // println("[x] trigger channel number cannot be greater than 7.");

               codeword_t trig_mask;
               trig_mask.set(7-trig_channel);

               trigger * t0 = new trigger(trig_mask,trig_width,operation_type,latency);
               trigger * t1 = new trigger(trig_mask,trig_width,operation_type,latency);
               t0->qasm_label = qasm_label;
               t1->qasm_label = qasm_label;
               instructions.push_back(t0);
               instructions.push_back(t1);
            }

            /**
             * decompose trigger sequence
             */
            qumis_program_t decompose()
            {
               instructions[0]->start   = start;
               instructions[0]->latency = latency;
               instructions[1]->start   = start+duration;
               instructions[1]->latency = latency;
               #if 0
               println("trigger_sequence::decompose() :");
               println("\t trigger_sequence[0] latency : " << instructions[0]->latency);
               println("\t trigger_sequence[0] start   : " << instructions[0]->start);
               println("\t trigger_sequence[1] latency : " << instructions[1]->latency);
               println("\t trigger_sequence[1] start   : " << instructions[1]->start);
               #endif
               return instructions;
            }

            /**
             * set used qubits
             */
            void set_used_qubits(qubit_set_t& uq)
            {
               used_qubits = uq;
               for (auto i : instructions)
                  i->used_qubits = uq;
            }


            /**
             *  compensate for latency
             */
            void compensate_latency()
            {
               if (!latency_compensated)
               {
                  // println("compensate latency : " << start << " -> " << (start-latency) << " : latency = " << latency);
                  start -= latency;
                  for (auto i : instructions)
                     i->compensate_latency();
                  latency_compensated = true;
               }
               else
               {
                  println("[x] warning : latency of instruction '" << this << "' is already compensated !");
               }
            }


            /**
             * generate code
             */
            qumis_instr_t code()
            {
               std::stringstream instr;
               instr << instructions[0]->code() << "\nwait " << duration+trig_width << "\n";
               instr << instructions[1]->code() << "\nwait " << trig_width;
               return instr.str();
            }

            /**
             * trace
             */
            instruction_traces_t trace()
            {
               instruction_traces_t trs;
               size_t latent_start = (latency_compensated ? (start) : (start-latency));
               std::string label   = qasm_label + " : " + code();
               // println("qasm label   : " << label);
               // println("start        : " << start);
               // println("latent start : " << latent_start);

               for (size_t i=0; i<instructions.size(); ++i)
               {
                  instruction_trace_t t  = { trig_channel, label, start, (start+instructions[i]->duration), "#DD5437", __top_pos__ };
                  instruction_trace_t lt = { trig_channel, label, latent_start, (latent_start+instructions[i]->duration), "#808080", __bottom_pos__ };
                  trs.push_back(lt);
                  trs.push_back(t);
               }

               return trs;
            }

            /**
             * set start
             */
            void set_start(size_t t)
            {
               start = t;
               instructions[0]->start = t;
               instructions[1]->start = t+trig_width+duration;
            }

      };






      /**
       * measure
       */
      class measure : public qumis_instruction
      {
         public:

            qumis_instruction * instruction;

         public:

            /**
             * ctor
             */
            measure(qumis_instruction * instruction, size_t duration, size_t latency=0) : instruction(instruction)
            {
               this->duration = duration;
               this->latency  = latency;
               operation_type = __measurement__;
               used_resources.set();
            }

            /**
             * generate code
             */
            qumis_instr_t code()
            {
               std::stringstream inst;
               inst << instruction->code(); // << "\n";
               // inst << "wait " << (duration-instruction->duration);
               return inst.str();
            }

            /**
             *  compensate for latency
             */
            void compensate_latency()
            {
               if (!latency_compensated)
               {
                  // println("compensate latency : " << start << " -> " << (start-latency) << " : latency = " << latency);
                  start -= latency;
                  latency_compensated = true;
                  instruction->compensate_latency();
               }
               else
               {
                  println("[x] warning : latency of instruction '" << this << "' is already compensated !");
               }
            }


            /**
             * trace
             */
            instruction_traces_t trace()
            {
               instruction_traces_t itrs = instruction->trace();
               instruction_traces_t trs;
               size_t latent_start = (latency_compensated ? (start) : (start-latency));
               bool   lt = true;
               for (instruction_trace_t t : itrs)
               {
                  t.start = (lt ? latent_start : start);
                  t.end   = (lt ? (latent_start+duration) : (start+duration));
                  t.label = qasm_label + " : " + code();
                  t.color = (lt ? "#808080" : "#328F1C");
                  trs.push_back(t);
                  lt = !lt;
               }
               return trs;
            }


            /**
             * set start
             */
            void set_start(size_t t)
            {
               start = t;
               instruction->set_start(t);
            }

      };

      /**
       * qumis comparator
       */
      static bool qumis_comparator(qumis_instruction * i1, qumis_instruction * i2)
      {
         return (i1->start < i2->start);
      }

      /**
       * triggers comparator
       */
      static bool triggers_comparator(qumis_instruction * t1, qumis_instruction * t2)
      {
         return (t1->duration < t2->duration);
      }



   }
}

#endif // QL_QUMIS_H


