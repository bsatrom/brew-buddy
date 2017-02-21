const Joi = require('joi');

const optsSchema = Joi.object().keys({
  api: Joi.object().required().keys({
    uri: Joi.string().required(),
    headers: Joi.object().required().keys({
      "Content-Type": Joi.string().required()
    }),
    payload: {
      device_id: Joi.string().required(),
      temperature: Joi.string().required(),
      brew_id: Joi.string().required(),
      brew_stage: Joi.string().required(),
      date_created: Joi.string().required()
    }
  }),
  lowTemp: Joi.number().required(),
  highTemp: Joi.number().required(),
  interval: Joi.number().required(),
  totalDuration: Joi.number().required(),
  strategy: Joi.string().required()
})

module.exports = optsSchema;
