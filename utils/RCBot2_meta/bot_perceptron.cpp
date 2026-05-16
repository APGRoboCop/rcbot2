// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
/*
 *    This file is part of RCBot.
 *
 *    RCBot by Paul Murphy adapted from Botman's HPB Bot 2 template.
 *
 *    RCBot is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    RCBot is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RCBot; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

#include <cmath>
#include <mem.h>
#include <cstring>
//#include "vstdlib/random.h" // for random functions
#include "bot_mtrand.h"
#include "bot_perceptron.h"

#include "logging.h"

ga_nn_value CPerceptron::m_fDefaultLearnRate = 0.5f;
ga_nn_value CPerceptron::m_fDefaultBias = 1.0f;

CNeuron :: CNeuron ()
{
	m_iInputs = 0;
	m_LearnRate = 0;
	m_output = 0;
	m_Bias = 0;
	
	m_weights = nullptr;
	m_inputs = nullptr;
}

CPerceptron::CPerceptron(const unsigned short iInputs)
{
	m_inputs = new ga_nn_value [iInputs];//.clear();
	m_weights = new ga_nn_value[iInputs];
	m_iInputs = iInputs;
	
	m_LearnRate = 0.4f;
	// bias weight
	m_Bias = -0.5;
	
	for (unsigned short int i = 0; i < m_iInputs; i++)
		m_weights[i] = -0.3f + randomFloat(0.0f, 0.6f);
}

void CPerceptron::setWeights(const ga_nn_value* weights) const {
	if (!weights) {
		logger->Log(LogLevel::WARN, "Weights pointer is null");
		return; // Exit the function to prevent unsafe usage
	}

	std::memcpy(m_weights, weights, sizeof(ga_nn_value) * m_iInputs);
}

void CNeuron :: input ( ga_nn_value *inputs )
{
	std::memcpy(m_inputs,inputs,sizeof(ga_nn_value)*m_iInputs);
}

ga_nn_value CNeuron::execute()
{
	return ga_nn_value();
}

bool CNeuron::fired() //TODO: experimental [APG]RoboCop[CL]
{
	return false;
}

ga_nn_value CPerceptron :: execute ()
{
	static unsigned short i;
	static ga_nn_value *w;
	static ga_nn_value *x;
	// bias weight
	m_output = m_Bias;

	w = m_weights;
	x = m_inputs;
	
	for ( i = 0; i < m_iInputs; i ++ )
	{
		m_output += *w * *x;
		w++;
		x++;
	}
	
	// sigmoid function
	m_output = 1.0f / (1.0f + std::exp(-m_output)); //m_transferFunction->transfer(fNetInput);
	
	return m_output;
}

bool CPerceptron :: fired () const
{
	return m_output >= 0.5f;
}

ga_nn_value CPerceptron :: getOutput () const
{
	return m_output;
}

void CPerceptron::train(const ga_nn_value expectedOutput)
{
	static unsigned short i;
	static ga_nn_value *w;
	static ga_nn_value *x;

	w = m_weights;
	x = m_inputs;

	// bias
	m_Bias += m_LearnRate*(expectedOutput-m_output);
	
	for ( i = 0; i < m_iInputs; i ++ )
	{
		*w = *w + m_LearnRate*(expectedOutput-m_output)* *x;
		w++;
		x++;
	}
}

void CLogisticalNeuron :: train ()// ITransfer *transferFunction, bool usebias )
{
	static unsigned short i;
	static ga_nn_value *w;
	static ga_nn_value *x;
	static ga_nn_value delta;

	w = m_weights;
	x = m_inputs;

	for ( i = 0; i < m_iInputs; i ++ )
	{
		delta = m_LearnRate * *x * m_error;
		delta += m_momentum * 0.9f;
		*w = *w + delta;
		m_momentum = delta;
		x++;
		w++;
	}

	m_Bias += m_LearnRate * m_error;
}

ga_nn_value CLogisticalNeuron :: execute ( )//, bool usebias )
{
	static unsigned short int i;
	static ga_nn_value *w;
	static ga_nn_value *x;

	//m_netinput = 0;
	w = m_weights;
	x = m_inputs;

	// bias weight
	m_netinput = m_Bias;//m_netinput assigned twice? [APG]RoboCop[CL]
	
	for ( i = 0; i < m_iInputs; i ++ )	
	{
		m_netinput += *w * *x;
		w++;
		x++;
	}

	m_output = 1/(1+std::exp(-m_netinput));//transferFunction->transfer(m_netinput);
	
	return m_output;
}

void CLogisticalNeuron::init(const unsigned short iInputs, const ga_nn_value learnrate)
{
		m_weights = new ga_nn_value[iInputs];
		m_inputs = new ga_nn_value[iInputs];

		m_error = 0;
		m_netinput = 0;
		m_LearnRate = 0.2f;
		m_output = 0;
		m_momentum = 0;

		// initialise?
		for ( unsigned short int i = 0; i < iInputs; i ++ )
			m_weights[i] = randomFloat(-0.99f,0.99f);

		m_iInputs = iInputs;
		m_LearnRate = learnrate;
}

CBotNeuralNet :: CBotNeuralNet (const unsigned short numinputs, const unsigned short numhiddenlayers,
							  const unsigned short neuronsperhiddenlayer, const unsigned short numoutputs,
								const ga_nn_value learnrate)
{
	unsigned short i;

	m_pOutputs = new CLogisticalNeuron[numoutputs];
	m_pHidden = new CLogisticalNeuron*[numhiddenlayers];

	m_layerinput = new ga_nn_value[_MAX(numinputs,neuronsperhiddenlayer)];
	m_layeroutput = new ga_nn_value[_MAX(numoutputs,_MAX(numinputs,neuronsperhiddenlayer))];

	for ( unsigned short j = 0; j < numhiddenlayers; j ++ )
	{
		m_pHidden[j] = new CLogisticalNeuron[neuronsperhiddenlayer];

		for ( i = 0; i < neuronsperhiddenlayer; i ++ )
		{
			if ( j == 0 )
				m_pHidden[j][i].init(numinputs,learnrate);
			else
				m_pHidden[j][i].init(neuronsperhiddenlayer,learnrate);
		}
	}

	for ( i = 0; i < numoutputs; i ++ )
		m_pOutputs[i].init(neuronsperhiddenlayer,learnrate);

	//m_transferFunction = new CSigmoidTransfer ();

	m_numInputs = numinputs;
	m_numOutputs = numoutputs;
	m_numHidden = neuronsperhiddenlayer;
	m_numHiddenLayers = numhiddenlayers;
}

constexpr int RCPP_VERB_EPOCHS = 1000;

void CBotNeuralNet :: batch_train (const CTrainingSet *tset, const unsigned short epochs) const
{
	unsigned short i; // ith node
	unsigned short j; //jth output
	CLogisticalNeuron*pOutputNode;
	const unsigned short numbatches = tset->getNumBatches();
	const training_batch_t *batches = tset->getBatches();
	const ga_nn_value min_value = tset->getMinScale();
	const ga_nn_value max_value = tset->getMaxScale();

	ga_nn_value* outs = new ga_nn_value [m_numOutputs];

	for (unsigned short e = 0; e < epochs; e++)
	{
		/*if ( !(e%RCPP_VERB_EPOCHS) )
		{
			system("CLS");
			printf("-----epoch %d-----\n",e);
			printf("training... %0.1f percent",(((float)e/epochs))*100);
			printf("in1\tin2\texp\tact\terr\n");
		}*/

		for ( unsigned short bi = 0; bi < numbatches; bi ++ )
		{
			std::memset(outs,0,sizeof(ga_nn_value)*m_numOutputs);

			execute(batches[bi].in,outs,min_value,max_value);

			CLogisticalNeuron* pNode = m_pOutputs;

			// work out error for output layer
			for (j = 0; j < m_numOutputs; j++)
			{
				const ga_nn_value act_out = pNode->getOutput();
				const ga_nn_value exp_out = batches[bi].out[j];
				const ga_nn_value out_error = act_out * (1.0f - act_out) * (exp_out - act_out);
				pNode->setError(out_error);
				pNode++;
			}

			/*if (  !(e%RCPP_VERB_EPOCHS) )
			{
				printf("%0.2f\t%0.2f\t%0.2f\t%0.6f\t%0.6f\n",batches[bi].in[0],batches[bi].in[1],batches[bi].out[0],outs[0],out_error);
			}*/

			pNode = m_pHidden[m_numHiddenLayers-1];

			//Send Error back to Hidden Layer before output
			for (i = 0; i < m_numHidden; i++)
			{	
				ga_nn_value err = 0;
				pOutputNode = m_pOutputs;

				for (j = 0; j < m_numOutputs; j++)
				{
					err += pOutputNode->getError(i);
					pOutputNode++;
				}

				//pNode = &m_pHidden[m_numHiddenLayers-1][i];

				pNode->setError(pNode->getOutput() * (1.0f-pNode->getOutput()) * err);
				pNode++;
			}

			for (signed short l = m_numHiddenLayers - 2; l >= 0; l--) //l should be int only? [APG]RoboCop[CL]
			{
				pOutputNode = m_pHidden[l];
				//Send Error back to Input Layer
				for (i = 0; i < m_numHidden; i++)
				{	
					ga_nn_value err = 0;

					pNode = m_pHidden[l+1];

					for (j = 0; j < m_numHidden; j++)
					{
						// check the error from the next layer
						err += pNode->getError(i);
						pNode++;
					}

					pOutputNode->setError(pOutputNode->getOutput() * (1.0f-pOutputNode->getOutput()) * err);
					pOutputNode ++;
				}
			}

			for (j = 0; j < m_numHiddenLayers; j++)
			{
				pNode = m_pHidden[j];
				// update weights for hidden layer (each neuron)
				for (i = 0; i < m_numHidden; i++)
				{	
					pNode->train(); // update weights for this node
					pNode++;
				}
			}

			// update weights for output layer
			for ( i = 0; i < m_numOutputs; i ++ )
			{	
				m_pOutputs[i].train(); // update weights for this node
			}
		}
	}

	delete[] outs;
}

void CBotNeuralNet :: execute (const ga_nn_value* inputs, ga_nn_value* outputs, const ga_nn_value fMin, const ga_nn_value fMax) const
{
	static CLogisticalNeuron *pNode;
	static CLogisticalNeuron *pLayer; //Unused? [APG]RoboCop[CL]
	static unsigned short i; // i-th node
	static unsigned short l; // layer
	static ga_nn_value *output_it;

	std::memset(outputs,0,sizeof(ga_nn_value)*m_numOutputs);
	std::memset(m_layeroutput,0,sizeof(ga_nn_value)*m_numInputs);
	std::memcpy(m_layerinput,inputs,sizeof(ga_nn_value)*m_numInputs);

	// Missing inputs!!!
	for (i = 0; i < m_numInputs; i++)
		m_layerinput[i] = inputs[i];

	for ( l = 0; l < m_numHiddenLayers; l ++ )
	{
		output_it = m_layeroutput;

		//pLayer = m_pHidden[l];

		pNode = m_pHidden[l];

		// execute hidden
		for ( i = 0; i < m_numHidden; i ++ )
		{
			pNode->input(m_layerinput);
			pNode->execute();//m_transferFunction);

			*output_it = pNode->getOutput();
			output_it ++;

			pNode++; // next
			//layeroutput.emplace_back(pNode->getOutput());
		}

		std::memcpy(m_layerinput,m_layeroutput,sizeof(ga_nn_value) * m_numHidden);

		//pLayer++;
	}

	// execute output
	for ( i = 0; i < m_numOutputs; i ++ )
	{
		pNode = &m_pOutputs[i];
		pNode->input(m_layeroutput);
		pNode->execute();//m_transferFunction);
		outputs[i] = gdescale(pNode->getOutput(),fMin,fMax);
	}
}