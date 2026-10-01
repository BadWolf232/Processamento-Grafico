    glUniform3f(corLoc, 1.0f, 0.0f, 0.0f);
    transformacao = glm::mat4(1.0f);
    transformacao = glm::translate(transformacao, glm::vec3(-8.0f, 0.0f, -10.0f));
    transformacao = glm::rotate(transformacao, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    transformacao = glm::scale(transformacao, glm::vec3(4.0f, 1.0f, 15.0f)); 
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transformacao));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    transformacao = glm::mat4(1.0f);
    transformacao = glm::translate(transformacao, glm::vec3(5.0f, 0.0f, -5.0f));
    transformacao = glm::rotate(transformacao, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    transformacao = glm::scale(transformacao, glm::vec3(2.0f, 2.0f, 5.0f));
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transformacao));
    glDrawArrays(GL_TRIANGLES, 0, 36);

    transformacao = glm::mat4(1.0f);
    transformacao = glm::translate(transformacao, glm::vec3(0.0f, 0.0f, -8.0f));
    transformacao = glm::rotate(transformacao, (float)glfwGetTime(), glm::vec3(0.5f, 1.0f, 0.0f));
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transformacao));
    glDrawArrays(GL_TRIANGLES, 0, 36);